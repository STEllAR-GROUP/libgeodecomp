#ifndef LIBGEODECOMP_STORAGE_POINTERNEIGHBORHOOD_H
#define LIBGEODECOMP_STORAGE_POINTERNEIGHBORHOOD_H

#include <libgeodecomp/geometry/fixedcoord.h>
#include <libgeodecomp/geometry/stencils.h>

namespace LibGeoDecomp {

/**
 * provides a neighborhood which can be adressed by parameters known
 * at compile time. It uses an array of pointers to access the cells,
 * which makes it suitable for any topology and storage. Short-lived
 * instances should be optimized away by the compiler.
 */
template<typename CELL, typename STENCIL>
class PointerNeighborhood
{
public:
    explicit PointerNeighborhood(CELL **cells) :
        cells(cells)
    {}

    template<int X, int Y, int Z>
    const CELL& operator[](FixedCoord<X, Y, Z>) const
    {
        return cells[Stencils::OffsetHelper<STENCIL, X, Y, Z>::VALUE][0];
    }

private:
    CELL **cells;
};

}

#endif
