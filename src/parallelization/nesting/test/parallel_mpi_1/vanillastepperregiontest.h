#include <libgeodecomp.h>
#include <libgeodecomp/io/testinitializer.h>
#include <libgeodecomp/misc/testhelper.h>
#include <libgeodecomp/parallelization/nesting/vanillastepper.h>
#include <libgeodecomp/storage/patchbuffer.h>

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

using namespace LibGeoDecomp;

namespace LibGeoDecomp {

typedef std::pair<Region<2>, unsigned> Event;

template<class GRID_TYPE1, class GRID_TYPE2>
class MockPatchBuffer : public PatchBuffer<GRID_TYPE1, GRID_TYPE2>
{
public:

    virtual void get(
        GRID_TYPE2& destinationGrid,
        const Region<2>& patchRegion,
        unsigned nanoStep)
    {
        events.push_back(Event(patchRegion, nanoStep));
        PatchBuffer<GRID_TYPE1, GRID_TYPE2>::get(
            &destinationGrid,
            patchRegion,
            nanoStep);
    }

    inline
    const std::vector<Event>& getEvents() const
    {
        return events;
    }

private:
    std::vector<Event> events;
};

class VanillaStepperRegionTest : public CxxTest::TestSuite
{
public:
    typedef VanillaStepper<TestCell<2>, UpdateFunctorHelpers::ConcurrencyEnableOpenMP> StepperType;
    typedef StepperType::GridType GridType;

    void setUp()
    {
        // Init simulation area
        ghostZoneWidth = 3;
        init.reset(new TestInitializer<TestCell<2> >(Coord<2>(17, 12)));
        CoordBox<2> rect = init->gridBox();

        // Set up a striping partition. We'll take rank 1, placing us
        // between the two others 0 and 2.
        std::vector<std::size_t> weights(3);
        weights[0] = 4*17 + 7;
        weights[1] = 2*17 - 1;
        weights[2] = 12*17 - weights[0] - weights[1];
        boost::shared_ptr<Partition<2> > partition(
            new StripingPartition<2>(Coord<2>(0, 0), rect.dimensions, 0, weights));

        boost::shared_ptr<AdjacencyManufacturer<2> > dummyAdjacencyManufacturer(new DummyAdjacencyManufacturer<2>);

        // Feed the partition into the partition manager
        partitionManager.reset(new PartitionManager<Topologies::Cube<2>::Topology>());
        partitionManager->resetRegions(
            dummyAdjacencyManufacturer,
            init->gridBox(),
            partition,
            1,
            ghostZoneWidth);

        std::vector<CoordBox<2> > boundingBoxes;
        std::vector<CoordBox<2> > expandedBoundingBoxes;
        for (int i = 0; i < 3; ++i) {
            Region<2> region = partition->getRegion(i);
            Region<2> expandedRegion = region.expandWithTopology(ghostZoneWidth, rect.dimensions, Topologies::Cube<2>::Topology());

            boundingBoxes.push_back(region.boundingBox());
            expandedBoundingBoxes.push_back(expandedRegion.boundingBox());
        }

        partitionManager->resetGhostZones(boundingBoxes, expandedBoundingBoxes);

        // The Unit Under Test: the stepper
        stepper.reset(new StepperType(partitionManager, init));
    }

    void testUpdate1()
    {
        checkInnerSet(0, 0);
        stepper->update1();
        checkInnerSet(1, 1);
        stepper->update1();
        checkInnerSet(2, 2);
    }

private:
    int ghostZoneWidth;
    boost::shared_ptr<TestInitializer<TestCell<2> > > init;
    boost::shared_ptr<PartitionManager<Topologies::Cube<2>::Topology> > partitionManager;
    boost::shared_ptr<StepperType> stepper;

    void checkInnerSet(
        unsigned shrink,
        unsigned expectedStep)
    {
        TS_ASSERT_TEST_GRID_REGION(
            GridType,
            stepper->grid(),
            partitionManager->innerSet(shrink),
            expectedStep);
    }
};

}
