/* vim:set expandtab tabstop=4 shiftwidth=4 softtabstop=4: */

#include <iostream>
#include <libgeodecomp/libgeodecomp.h>

#include <CL/cl.hpp>

using namespace LibGeoDecomp;

std::ostream& operator<< ( std::ostream& o, cl::Platform p ) {
    o << "CL_PLATFORM_VERSION\t= "     << p.getInfo<CL_PLATFORM_VERSION>()
      << std::endl
      << "CL_PLATFORM_NAME\t= "        << p.getInfo<CL_PLATFORM_NAME>()
      << std::endl
      << "CL_PLATFORM_VENDOR\t= "      << p.getInfo<CL_PLATFORM_VENDOR>()
      << std::endl
      << "CL_PLATFORM_EXTENSIONS\t= "  << p.getInfo<CL_PLATFORM_EXTENSIONS>();
    return o;
}

std::ostream& operator<< ( std::ostream& o, cl::Device d ) {
    o << "CL_DEVICE_EXTENSIONS\t\t\t= "
      << d.getInfo<CL_DEVICE_EXTENSIONS>()                << std::endl
      << "CL_DEVICE_GLOBAL_MEM_SIZE\t\t= "
      << d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()           << std::endl
      << "CL_DEVICE_LOCAL_MEM_SIZE\t\t= "
      << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()            << std::endl
      << "CL_DEVICE_MAX_CLOCK_FREQUENCY\t\t= "
      << d.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>()       << std::endl
      << "CL_DEVICE_MAX_COMPUTE_UNITS\t\t= "
      << d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()         << std::endl
      << "CL_DEVICE_MAX_CONSTANT_ARGS\t\t= "
      << d.getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>()         << std::endl
      << "CL_DEVICE_MAX_MEM_ALLOC_SIZE\t\t= "
      << d.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()        << std::endl
      << "CL_DEVICE_MAX_PARAMETER_SIZE\t\t= "
      << d.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>()        << std::endl
      << "CL_DEVICE_MAX_WORK_GROUP_SIZE\t\t= "
      << d.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()       << std::endl
      << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS\t= "
      << d.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>()  << std::endl
      << "CL_DEVICE_MAX_WORK_ITEM_SIZES\t\t= ";

    std::vector<size_t> wis = d.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    std::vector<size_t>::iterator it = wis.begin();

    o << "[";
    while ( it != wis.end() ) o << *it << ( ++it != wis.end() ? ", " : "" );
    o << "]"                                              << std::endl;

    o << "CL_DEVICE_NAME\t\t\t\t= "
      << d.getInfo<CL_DEVICE_NAME>()                      << std::endl
      << "CL_DEVICE_VENDOR\t\t\t= "
      << d.getInfo<CL_DEVICE_VENDOR>()                    << std::endl
      << "CL_DEVICE_VERSION\t\t\t= "
      << d.getInfo<CL_DEVICE_VERSION>()                   << std::endl
      << "CL_DRIVER_VERSION\t\t\t= "
      << d.getInfo<CL_DRIVER_VERSION>()                   << std::endl
      << "CL_DEVICE_EXTENSIONS\t\t\t= "
      << d.getInfo<CL_DEVICE_EXTENSIONS>();

    return o;
}

class Cell
{
public:
    class API :
        public APITraits::HasFixedCoordsOnlyUpdate,
        public APITraits::HasStencil<Stencils::VonNeumann<3, 1> >,
        public APITraits::HasCubeTopology<3>
    {};

    inline explicit Cell(const double& v = 0) :
        temp(v)
    {}

    template<typename COORD_MAP>
    void update(const COORD_MAP& neighborhood, const unsigned& /* nanoStep */)
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

template<typename CELL>
class MyFutureOpenCLStepper
{
public:
    typedef typename APITraits::SelectTopology<CELL>::Value Topology;
    typedef DisplacedGrid<CELL, Topology>  GridType;
    const static int DIM = Topology::DIM;

    MyFutureOpenCLStepper(const CoordBox<DIM> box) :
        box(box),
        hostGrid(box)
    {
       std::vector<cl::Platform> platforms;
       cl::Platform::get ( &platforms );

       std::cerr << "# of Platforms: " << platforms.size() << std::endl;

       for ( auto & platform : platforms )
           std::cerr << platform;

       std::vector<cl::Device> devices;

       platforms[0].getDevices ( CL_DEVICE_TYPE_ALL, &devices );

       std::cerr << "# of Devices: " << devices.size() << std::endl;

       for ( auto & device : devices )
           std::cerr << device;

        // todo: allocate deviceGridOld, deviceGridNew via OpenCL on device
        // todo: specify OpenCL platform, device via constructor
    }

    void regionToVec(const Region<DIM>& region, SuperVector<int> *coordsX, SuperVector<int> *coordsY, SuperVector<int> *coordsZ)
    {
        // todo: iterate through region and add all coordinates to the corresponding vectors
    }

    template<typename GRID>
    void setGridRegion(const GRID& grid, const Region<DIM>& region)
    {
        // todo: copy all coords in region from grid (on host) to deviceGridOld
    }

    template<typename GRID>
    void getGridRegion(const GRID *grid, const Region<DIM>& region)
    {
        // todo: copy all coords in region from deviceGridOld (on host) to grid
    }

private:
    CoordBox<DIM> box;
    GridType hostGrid;
    // fixme deviceGridOld;
    // fixme deviceGridNew;
};

int main(int argc, char **argv)
{
    MyFutureOpenCLStepper<Cell> stepper(
        CoordBox<3>(
            Coord<3>(10, 10, 10),
            Coord<3>(20, 30, 40)));
    std::cout << "test: " << sizeof(stepper) << "\n";
    return 0;
}
