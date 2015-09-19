// vim: noai:ts=4:sw=4:expandtab
#include <libgeodecomp/misc/autotuningsimulator.h>
#include <libgeodecomp/io/logger.h>
#include <libgeodecomp/misc/patternoptimizer.h>
#include <libgeodecomp/misc/simplexoptimizer.h>
#include <libgeodecomp/misc/simulationparameters.h>
#include <libgeodecomp/misc/simulationfactory.h>
#include <libgeodecomp/io/simpleinitializer.h>
#include <libgeodecomp/io/ppmwriter.h>
#include <libgeodecomp/io/simplecellplotter.h>
#include <libgeodecomp/io/tracingwriter.h>
#include <libgeodecomp/parallelization/serialsimulator.h>
#include <libgeodecomp/parallelization/cacheblockingsimulator.h>
#include <libgeodecomp/misc/simulationparameters.h>
#include <libgeodecomp/io/mpiiowriter.h>
//#include <libgeodecomp/parallelization/cudasimulator.h>
#define LIBGEODECOMP_DEBUG_LEVEL 4 

using namespace LibGeoDecomp;

class SimFabTestCell
{
public:
    class API :
        public APITraits::HasFixedCoordsOnlyUpdate,
        public APITraits::HasStencil<Stencils::VonNeumann<3, 1> >,
        public APITraits::HasTorusTopology<3>,
        public APITraits::HasSeparateCUDAUpdate,
        public APITraits::HasPredefinedMPIDataType<double>
    {};

    inline explicit SimFabTestCell(double v = 0) : temp(v)
    {}

    template<typename COORD_MAP>
    __host__
    void update(const COORD_MAP& neighborhood, const unsigned& nanoStep)
    {
        temp = (neighborhood[FixedCoord< 0,  0, -1>()].temp +
                neighborhood[FixedCoord< 0, -1,  0>()].temp +
                neighborhood[FixedCoord<-1,  0,  0>()].temp +
                neighborhood[FixedCoord< 1,  0,  0>()].temp +
                neighborhood[FixedCoord< 0,  1,  0>()].temp +
                neighborhood[FixedCoord< 0,  0,  1>()].temp) * (1.0 / 6.0);
    }

    template<typename COORD_MAP>
    __device__
    void updateCuda(const COORD_MAP& neighborhood, const unsigned& nanoStep)
    {
        temp = (neighborhood[FixedCoord< 0,  0, -1>()].temp +
                neighborhood[FixedCoord< 0, -1,  0>()].temp +
                neighborhood[FixedCoord<-1,  0,  0>()].temp +
                neighborhood[FixedCoord< 1,  0,  0>()].temp +
                neighborhood[FixedCoord< 0,  1,  0>()].temp +
                neighborhood[FixedCoord< 0,  0,  1>()].temp) * (1.0 / 6.0);
    }

    double temp;
};

class SimFabTestInitializer : public SimpleInitializer<SimFabTestCell>
{
public:
    SimFabTestInitializer(Coord<3> gridDimensions = Coord<3>(100,100,100), unsigned timeSteps = 100) :
        SimpleInitializer<SimFabTestCell>(gridDimensions, timeSteps)
    {}

    virtual void grid(GridBase<SimFabTestCell, 3> *target)
    {
        int counter = 0;

        CoordBox<3> box = target->boundingBox();
        for (CoordBox<3>::Iterator i = box.begin(); i != box.end(); ++i) {
            target->set(*i, SimFabTestCell(++counter));
        }
    }
};

class AutotuningSimulatorTest : public CxxTest::TestSuite
{
public:
    void setUp()
    {
        dim = Coord<3>(100,100,100);
        maxSteps = 100;
    }

    void tearDown()
    {
    }

    void xtestBasicPatternOptimized()
    {
        LOG(Logger::INFO, "AutotuningSimulatorTest::TestBasicPatternOptimized()")
        AutoTuningSimulator<SimFabTestCell, PatternOptimizer> ats(
            SimFabTestInitializer(dim, maxSteps));
        ats.setSimulationSteps(4);
        ats.run();
    }

    void xtestBasicSimplexOptimized()
    {
        LOG(Logger::INFO, "AutotuningSimulatorTest::testBasicSimplexOptimized()")
        AutoTuningSimulator<SimFabTestCell, SimplexOptimizer> ats(
            SimFabTestInitializer(dim, maxSteps));
        ats.run();
    }

    void xtestAddOwnSimulations()
    {
        LOG(Logger::INFO, "AutotuningSimulationTest::testAddOwnSimulations()")
        AutoTuningSimulator<SimFabTestCell, PatternOptimizer> ats(
            SimFabTestInitializer(dim, maxSteps));
        ats.deleteAllSimulations();
        SimulationParameters params;
        params.addParameter("WavefrontWidth", 1, 300);
        params.addParameter("WavefrontHeight", 1, 300);
        params.addParameter("PipelineLength", 1, 25);
        AutoTuningSimulator<SimFabTestCell, PatternOptimizer>::Result newResult(
            "CacheBlockingSimulation",
            params);
        ats.addNewSimulation(newResult, "1.CacheBlockingSimulator",
            SimFabTestInitializer(dim,maxSteps));
        ats.run();
    }

    void xtestManuallyParamterized()
    {
        LOG(Logger::INFO, "AutotuningSimulatorTest:test:ManuallyParameterized()")
        AutoTuningSimulator<SimFabTestCell, PatternOptimizer> ats(
            SimFabTestInitializer(dim, maxSteps));
        
        SimulationParameters params;
        std::vector<std::string> simTypes;
        simTypes << "CacheBlockingSimulator";
        params.addParameter("Simulator" , simTypes);
        params.addParameter("WavefrontWidth", 1, 300);
        params.addParameter("WavefrontHeight", 1, 300);
        params.addParameter("PipelineLength", 1, 25);
        
        ats.setParameters(params, "CacheBlockingSimulation");
        ats.run();

    }

private:
    Coord<3> dim;
    unsigned maxSteps;
};

class SimulationFactoryTest : public CxxTest::TestSuite
{

public:
    void setUp()
    {
        dim = Coord<3>(100,100,100);
        maxSteps = 100;
        cfab = new CacheBlockingSimulationFactory<SimFabTestCell>(
                    SimFabTestInitializer(dim, maxSteps));
        fab = new SerialSimulationFactory<SimFabTestCell>(
                    SimFabTestInitializer(dim, maxSteps));
    }

    void tearDown()
    {
        delete fab;
    }

    void testBasic()
    {
        LOG(Logger::INFO, "SimulationFactoryTest::testBasic")
        for (int i =1;i<=2;i++)
        {
#ifdef WIHT_CUDA
            std::cout << "Cuda is active" << std::endl;
#endif
            Simulator<SimFabTestCell> *sim = fab->operator()();
            sim->run();
            delete sim;
        }
    }

    void testCacheBlockingFitness()
    {
        LOG(Logger::INFO, "SimulationFactoryTest::testCacheBlockingFitness()")
        for(int i = 1;i <= 2; i++){
            cfab->parameters()["PipelineLength"].setValue(1);
            cfab->parameters()["WavefrontWidth"].setValue(100);
            cfab->parameters()["WavefrontHeight"].setValue(40);
            double fitness = cfab->operator()(cfab->parameters());
            LOG(Logger::INFO,  i << " fitness: " << fitness )
        }
    }
    
    void xtestAddWriterToSimulator()
    {
        LOG(Logger::INFO, "SimulationFactoryTest::testAddWriterToSimulator()")
        CacheBlockingSimulator<SimFabTestCell> *sim =  (
            CacheBlockingSimulator<SimFabTestCell> *)cfab->operator()();
        sim->addWriter(new TracingWriter<SimFabTestCell>(1, 100));
        sim->run();
        double fitness = cfab->operator()(cfab->parameters());
    }

    void xtestAddWriterToSerialSimulationFactory()
    {
        LOG(Logger::INFO, "SimulationFactoryTest::testAddWriterToSerialSimulationFactory()")
        Writer<SimFabTestCell> *writer = new TracingWriter<SimFabTestCell>(1, 100);
        fab->addWriter(*writer);
        fab->operator()(fab->parameters());
    }

    void xtestAddWriterToCacheBlockingSimulationFactory()
    {
        LOG(Logger::INFO, "SimulationFactoryTest::testAddWriterToCacheBlockingSimulationFactory()")
        Writer<SimFabTestCell> *writer = new TracingWriter<SimFabTestCell>(1, 100);
        cfab->addWriter(*writer);
        cfab->operator()(cfab->parameters());
    }



private:
    Coord<3> dim;
    unsigned maxSteps;
    SimulationFactory<SimFabTestCell> *fab, *cfab;
};
