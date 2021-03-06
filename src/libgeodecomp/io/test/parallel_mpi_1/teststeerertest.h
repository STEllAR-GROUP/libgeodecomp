#include <libgeodecomp/io/testinitializer.h>
#include <libgeodecomp/io/teststeerer.h>
#include <libgeodecomp/misc/sharedptr.h>
#include <libgeodecomp/misc/testhelper.h>
#include <libgeodecomp/parallelization/serialsimulator.h>

#include <cxxtest/TestSuite.h>

using namespace LibGeoDecomp;

namespace LibGeoDecomp {

class TestSteererTest : public CxxTest::TestSuite
{
public:
    typedef TestSteerer<2> SteererType;

    void setUp()
    {
        simulator.reset(
            new SerialSimulator<TestCell<2> >(
                new TestInitializer<TestCell<2> >(
                    Coord<2>(10, 20),
                    34,
                    10)));
        simulator->addSteerer(new SteererType(5, 15, 4 * 27));
    }

    void tearDown()
    {
        simulator.reset();
    }

    void testCycleJump()
    {
        simulator->run();

        TS_ASSERT_TEST_GRID(SerialSimulator<TestCell<2> >::GridBaseType,
                            *simulator->getGrid(),
                            (34 + 4) * 27);

    }

private:
    SharedPtr<SerialSimulator<TestCell<2> > >::Type simulator;
};

}
