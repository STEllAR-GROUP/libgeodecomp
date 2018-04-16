#ifndef LIBGEODECOMP_PARALLELIZATION_NESTING_STEERERADAPTER_H
#define LIBGEODECOMP_PARALLELIZATION_NESTING_STEERERADAPTER_H

#include <libgeodecomp/io/steerer.h>
#include <libgeodecomp/storage/patchprovider.h>

namespace LibGeoDecomp {

/**
 * Ths adapter is used to interface a Steerer with a Stepper. To the
 * Stepper this class appears like a PatchProvider. This allows us to
 * do computational steering with hierarchical Simulators (e.g.
 * HPXSimulator and HiParSimulator).
 */
template<typename GRID_TYPE, typename CELL_TYPE>
class SteererAdapter : public PatchProvider<GRID_TYPE>
{
public:
    typedef typename APITraits::SelectTopology<CELL_TYPE>::Value Topology;
    typedef typename SharedPtr<Steerer<CELL_TYPE> >::Type SteererPtr;

    static const unsigned NANO_STEPS = APITraits::SelectNanoSteps<CELL_TYPE>::VALUE;
    static const int DIM = Topology::DIM;

    using PatchProvider<GRID_TYPE>::storedNanoSteps;
    using PatchProvider<GRID_TYPE>::get;

    SteererAdapter(
        SteererPtr steerer,
        const std::size_t firstStep,
        const std::size_t lastStep,
        bool lastCall) :
        steerer(steerer),
        firstNanoStep(firstStep * NANO_STEPS),
        lastNanoStep(lastStep   * NANO_STEPS),
        lastCall(lastCall)
    {
        std::size_t firstRegularEventStep = firstStep;
        std::size_t period = steerer->getPeriod();
        std::size_t offset = firstStep % period;
        firstRegularEventStep = firstStep + period - offset;

        storedNanoSteps << firstNanoStep;
        storedNanoSteps << firstRegularEventStep * NANO_STEPS;
        storedNanoSteps << lastNanoStep;
    }

    virtual void setRegion(const Region<DIM>& region)
    {
        steerer->setRegion(region);
    }

    virtual void get(
        GRID_TYPE *destinationGrid,
        const Region<DIM>& patchableRegion,
        const Coord<DIM>& globalGridDimensions,
        const std::size_t globalNanoStep,
        const std::size_t rank,
        const bool remove = true)
    {
        std::size_t nanoStep = globalNanoStep % NANO_STEPS;
        if (nanoStep != 0) {
            throw std::logic_error(
                "SteererAdapter expects to be called only at the beginning of a time step (nanoStep == 0)");
        }

        std::size_t step = globalNanoStep / NANO_STEPS;

        SteererEvent event = STEERER_NEXT_STEP;
        if (globalNanoStep == firstNanoStep) {
            event = STEERER_INITIALIZED;
        }
        if (globalNanoStep == lastNanoStep) {
            event = STEERER_ALL_DONE;
        }
        if (globalNanoStep > lastNanoStep) {
            return;
        }

        if ((event == STEERER_NEXT_STEP) && (step % steerer->getPeriod() != 0)) {
            throw std::logic_error("SteererAdapter called at wrong step (got " + StringOps::itoa(step) +
                                   " but expected multiple of " + StringOps::itoa(steerer->getPeriod()));
        }

        typename Steerer<CELL_TYPE>::SteererFeedback feedback;

        steerer->nextStep(
            destinationGrid,
            patchableRegion,
            globalGridDimensions,
            step,
            event,
            rank,
            lastCall,
            &feedback);

        if (remove) {
            storedNanoSteps.erase(globalNanoStep);
            std::size_t stride = NANO_STEPS * steerer->getPeriod();
            std::size_t nextNanoStep =  globalNanoStep + stride;
            // first step might not be a multiple of the input period, so
            // this correction is required to get the input in sync with
            // global time steps.
            nextNanoStep -= (nextNanoStep % stride);
            storedNanoSteps << nextNanoStep;
        }

        // fixme: apply SteererFeedback!
    }

private:
    SteererPtr steerer;
    std::size_t firstNanoStep;
    std::size_t lastNanoStep;
    bool lastCall;
};

}

#endif
