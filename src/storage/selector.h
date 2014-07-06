#ifndef LIBGEODECOMP_STORAGE_SELECTOR_H
#define LIBGEODECOMP_STORAGE_SELECTOR_H

#include <libgeodecomp/config.h>
#include <libgeodecomp/io/logger.h>
#include <libgeodecomp/misc/apitraits.h>
#include <libgeodecomp/storage/defaultfilter.h>
#include <libgeodecomp/storage/filterbase.h>
#include <libflatarray/flat_array.hpp>
#include <typeinfo>

#ifdef LIBGEODECOMP_WITH_SILO
#include <silo.h>
#endif

#ifdef LIBGEODECOMP_WITH_MPI
#include <libgeodecomp/communication/typemaps.h>
#endif

namespace LibGeoDecomp {

class APITraits;

namespace SelectorHelpers {

template<typename CELL, typename MEMBER>
class GetMemberOffset
{
public:
    int operator()(MEMBER CELL:: *memberPointer, APITraits::TrueType)
    {
        return LibFlatArray::member_ptr_to_offset()(memberPointer);
    }

    int operator()(MEMBER CELL:: *memberPointer, APITraits::FalseType)
    {
        return -1;
    }
};

/**
 * Primitive datatypes don't have member pointers (or members in the
 * first place). So we provide this primitive implementation to copy
 * them en block.
 */
template<typename CELL>
class PrimitiveSelector
{
public:
    template<typename MEMBER>
    void operator()(const CELL *source, MEMBER *target, const std::size_t length) const
    {
        std::copy(source, source + length, target);
    }

    const std::string name() const
    {
        return "primitiveType";
    }

    std::size_t sizeOfMember() const
    {
        return sizeof(CELL);
    }

    std::size_t sizeOfExternal() const
    {
        return sizeof(CELL);
    }

    int offset() const
    {
        return 0;
    }

    void copyMemberIn(const char *source, CELL *target, int num) const
    {
        const CELL *actualSource = reinterpret_cast<const CELL*>(source);
        std::copy(actualSource, actualSource + num, target);
    }

    void copyMemberOut(const CELL *source, char *target, int num) const
    {
        CELL *actualTarget = reinterpret_cast<CELL*>(target);
        std::copy(source, source + num, actualTarget);
    }

    // intentionally leaving our copyStreak() as it should only be
    // called by SoAGrid, which isn't defined for primitive types
    // anyway.
};

}

/**
 * A Selector can be used by library code to extract data from user
 * code, e.g. so that writers can access a cell's member variable.
 *
 * The code is based on a rather obscure C++ feature. Thanks to Evan
 * Wallace for pointing this out:
 * http://madebyevan.com/obscure-cpp-features/#pointer-to-member-operators
 *
 * fixme: use this class in RemoteSteerer, VisItWriter...
 */
template<typename CELL>
class Selector
{
public:
    template<typename MEMBER>
    Selector(MEMBER CELL:: *memberPointer, const std::string& memberName) :
        memberPointer(reinterpret_cast<char CELL::*>(memberPointer)),
        memberSize(sizeof(MEMBER)),
        externalSize(sizeof(MEMBER)),
        memberOffset(typename SelectorHelpers::GetMemberOffset<CELL, MEMBER>()(
                         memberPointer,
                         typename APITraits::SelectSoA<CELL>::Value())),
        memberName(memberName),
        filter(new DefaultFilter<CELL, MEMBER, MEMBER>)
    {}

    Selector() :
        memberPointer(0),
        memberSize(0),
        externalSize(0),
        memberOffset(0),
        memberName("memberName not initialized")
    {}

    template<typename MEMBER>
    Selector(MEMBER CELL:: *memberPointer, const std::string& memberName, const boost::shared_ptr<FilterBase<CELL> >& filter) :
        memberPointer(reinterpret_cast<char CELL::*>(memberPointer)),
        memberSize(sizeof(MEMBER)),
        externalSize(filter->sizeOf()),
        memberOffset(typename SelectorHelpers::GetMemberOffset<CELL, MEMBER>()(
                         memberPointer,
                         typename APITraits::SelectSoA<CELL>::Value())),
        memberName(memberName),
        filter(filter)
    {}

    inline const std::string& name() const
    {
        return memberName;
    }

    inline std::size_t sizeOfMember() const
    {
        return memberSize;
    }

    inline std::size_t sizeOfExternal() const
    {
        return externalSize;
    }

    template<typename MEMBER>
    inline bool checkTypeID() const
    {
        return filter->checkExternalTypeID(typeid(MEMBER));
    }

    /**
     * The member's offset in LibFlatArray's SoA memory layout
     */
    inline int offset() const
    {
        return memberOffset;
    }

    /**
     * Read the data from source and set the corresponding member of
     * each CELL at target. Only useful for AoS memory layout.
     */
    inline void copyMemberIn(const char *source, CELL *target, int num) const
    {
        filter->copyMemberIn(source, target, num, memberPointer);
    }

    /**
     * Read the member of all CELLs at source and store them
     * contiguously at target. Only useful for AoS memory layout.
     */
    inline void copyMemberOut(const CELL *source, char *target, int num) const
    {
        filter->copyMemberOut(source, target, num, memberPointer);
    }

    /**
     * This is a helper function for writing members of a SoA memory
     * layout.
     */
    inline void copyStreakIn(const char *first, const char *last, char *target) const
    {
        filter->copyStreakIn(first, last, target);
    }

    /**
     * This is a helper function for reading members from a SoA memory
     * layout.
     */
    inline void copyStreakOut(const char *first, const char *last, char *target) const
    {
        filter->copyStreakOut(first, last, target);
    }

#ifdef LIBGEODECOMP_WITH_SILO
    int siloTypeID() const
    {
        return filter->siloTypeID();
    }
#endif

#ifdef LIBGEODECOMP_WITH_MPI
    MPI_Datatype mpiDatatype() const
    {
        return filter->mpiDatatype();
    }
#endif

    std::string typeName() const
    {
        return filter->typeName();
    }

    int arity() const
    {
        return filter->arity();
    }

    template<class Archive>
    void serialize(Archive& archive, const unsigned int version)
    {
        std::size_t *buf =
            reinterpret_cast<std::size_t*>(
                const_cast<char CELL::**>(&memberPointer));
        archive & *buf;

        archive & memberSize;
        archive & externalSize;
        archive & memberOffset;
        archive & memberName;
        archive & filter;
    }

private:
    char CELL:: *memberPointer;
    std::size_t memberSize;
    std::size_t externalSize;
    int memberOffset;
    std::string memberName;
    boost::shared_ptr<FilterBase<CELL> > filter;
};

/**
 * We provide these specializations to allow our unit tests to use
 * grids with primitive datatypes.
 */
template<>
class Selector<char> : public SelectorHelpers::PrimitiveSelector<char>
{
};

template<>
class Selector<int> : public SelectorHelpers::PrimitiveSelector<int>
{
};

template<>
class Selector<unsigned> : public SelectorHelpers::PrimitiveSelector<unsigned>
{
};

template<>
class Selector<float> : public SelectorHelpers::PrimitiveSelector<float>
{
};

template<>
class Selector<double> : public SelectorHelpers::PrimitiveSelector<double>
{
};

}

#endif