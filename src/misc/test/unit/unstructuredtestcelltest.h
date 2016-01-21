#include <libgeodecomp/io/unstructuredtestinitializer.h>
#include <libgeodecomp/misc/testhelper.h>
#include <libgeodecomp/misc/unstructuredtestcell.h>
#include <libgeodecomp/storage/unstructuredgrid.h>

using namespace LibGeoDecomp;

namespace LibGeoDecomp {

class UnstructuredTestCellTest : public CxxTest::TestSuite
{
public:
    void testBasic()
    {
        UnstructuredTestInitializer<> init(200, 60, 5);
        UnstructuredGrid<UnstructuredTestCell<> > grid(Coord<1>(200));
        init.grid(&grid);

        // fixme: needs test
    }
};

}
