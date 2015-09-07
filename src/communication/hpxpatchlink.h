#ifndef LIBGEODECOMP_COMMUNICATION_HPXPATCHLINK_H
#define LIBGEODECOMP_COMMUNICATION_HPXPATCHLINK_H

#include <libgeodecomp/communication/hpxreceiver.h>
#include <libgeodecomp/storage/gridvecconv.h>
#include <libgeodecomp/storage/patchaccepter.h>
#include <libgeodecomp/storage/patchprovider.h>

namespace LibGeoDecomp {

template <class GRID_TYPE>
class HPXPatchLink
{
public:
    friend class PatchLinkTest;

    typedef typename GRID_TYPE::CellType CellType;
    typedef typename SerializationBuffer<CellType>::BufferType BufferType;
    typedef typename SerializationBuffer<CellType>::FixedSize FixedSize;

    const static int DIM = GRID_TYPE::DIM;

    class Link
    {
    public:
        std::string genLinkName(const std::string& basename, std::size_t sourceRank, std::size_t targetRank)
        {
            return basename + "/PatchLink::/" +
                StringOps::itoa(sourceRank) + "-" + StringOps::itoa(targetRank);
        }

        /**
         * We'll use the linkName to uniquely identify the endpoints in AGAS.
         */
        inline Link(
            const Region<DIM>& region,
            const std::string& linkName) :
            linkName(linkName),
            lastNanoStep(0),
            stride(1),
            region(region),
            buffer(SerializationBuffer<CellType>::create(region))
        {}

        virtual ~Link()
        {
            wait();
        }

        /**
         * Should be called prior to destruction to allow
         * implementations to perform any cleanup actions (e.g. to
         * post any receives to pending transmissions).
         */
        virtual void cleanup()
        {}

        virtual void charge(std::size_t next, std::size_t last, std::size_t newStride)
        {
            lastNanoStep = last;
            stride = newStride;
        }

        inline void wait()
        {
            // MPI relict, unnecessary for HPX
        }

        inline void cancel()
        {
            // MPI relict, unnecessary for HPX
        }

    protected:
        std::string linkName;
        std::size_t lastNanoStep;
        long stride;
        Region<DIM> region;
        BufferType buffer;
    };

    class Accepter :
        public Link,
        public PatchAccepter<GRID_TYPE>
    {
    public:
        using Link::linkName;
        using Link::buffer;
        using Link::lastNanoStep;
        using Link::region;
        using Link::stride;
        using Link::wait;
        using PatchAccepter<GRID_TYPE>::checkNanoStepPut;
        using PatchAccepter<GRID_TYPE>::infinity;
        using PatchAccepter<GRID_TYPE>::pushRequest;
        using PatchAccepter<GRID_TYPE>::requestedNanoSteps;

        inline Accepter(
            const Region<DIM>& region,
            std::string basename,
            const std::size_t source,
            const std::size_t target) :
            Link(region, Link::genLinkName(basename, source, target)),
            receiverID(HPXReceiver<std::vector<CellType> >::find(linkName).get())
        {}

        virtual void charge(std::size_t next, std::size_t last, std::size_t newStride)
        {
            Link::charge(next, last, newStride);
            pushRequest(next);
        }

        virtual void put(
            const GRID_TYPE& grid,
            const Region<DIM>& /*validRegion*/,
            const std::size_t nanoStep)
        {
            std::cout << "put(" << nanoStep << ") validity: " << checkNanoStepPut(nanoStep) << "\n";
            if (!checkNanoStepPut(nanoStep)) {
                return;
            }

            GridVecConv::gridToVector(grid, &buffer, region);
            hpx::apply(typename HPXReceiver<std::vector<CellType> >::receiveAction(),
                       receiverID,  nanoStep, buffer);

            std::size_t nextNanoStep = (min)(requestedNanoSteps) + stride;
            if ((lastNanoStep == infinity()) ||
                (nextNanoStep < lastNanoStep)) {
                requestedNanoSteps << nextNanoStep;
            }

            erase_min(requestedNanoSteps);
        }

    private:
        hpx::id_type receiverID;
    };

    class Provider :
        public Link,
        public PatchProvider<GRID_TYPE>
    {
    public:
        using Link::linkName;
        using Link::buffer;
        using Link::lastNanoStep;
        using Link::region;
        using Link::stride;
        using Link::wait;
        using PatchProvider<GRID_TYPE>::checkNanoStepGet;
        using PatchProvider<GRID_TYPE>::infinity;
        using PatchProvider<GRID_TYPE>::storedNanoSteps;

        inline
        Provider(
            const Region<DIM>& region,
            std::string basename,
            const std::size_t source,
            const std::size_t target) :
            Link(region, Link::genLinkName(basename, source, target)),
            receiver(HPXReceiver<std::vector<CellType> >::make(linkName).get())
        {}

        virtual void cleanup()
        {
            // MPI relict, unnecessary for HPX
        }

        virtual void charge(const std::size_t next, const std::size_t last, const std::size_t newStride)
        {
            Link::charge(next, last, newStride);
            recv(next);
        }

        virtual void get(
            GRID_TYPE *grid,
            const Region<DIM>& patchableRegion,
            const std::size_t nanoStep,
            const bool remove = true)
        {
            if (storedNanoSteps.empty() || (nanoStep < (min)(storedNanoSteps))) {
                return;
            }

            checkNanoStepGet(nanoStep);

            GridVecConv::vectorToGrid(receiver->get(nanoStep).get(), grid, region);

            std::size_t nextNanoStep = (min)(storedNanoSteps) + stride;
            if ((lastNanoStep == infinity()) ||
                (nextNanoStep < lastNanoStep)) {
                recv(nextNanoStep);
            }

            erase_min(storedNanoSteps);
        }

        void recv(const std::size_t nanoStep)
        {
            storedNanoSteps << nanoStep;
        }

    private:
        boost::shared_ptr<HPXReceiver<std::vector<CellType> > > receiver;
    };
};

}

#endif