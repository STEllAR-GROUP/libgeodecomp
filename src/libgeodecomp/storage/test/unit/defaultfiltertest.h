#include <vector>

#include <cxxtest/TestSuite.h>
#include <libgeodecomp/misc/testcell.h>
#include <libgeodecomp/storage/defaultfilter.h>

using namespace LibGeoDecomp;

namespace LibGeoDecomp {

class DefaultFilterTest : public CxxTest::TestSuite
{
public:
    void testHostAoS()
    {
        std::vector<TestCell<2> > hostCellVec(40);
        std::vector<float> hostBuffer(40, -1);

        for (std::size_t i = 0; i < hostCellVec.size(); ++i) {
            hostCellVec[i].testValue = i + 0.5;
        }

        FilterBase<TestCell<2> > *filter = new DefaultFilter<TestCell<2>, float, float>();
        char TestCell<2>::* memberPointer = reinterpret_cast<char TestCell<2>::*>(&TestCell<2>::testValue);

        filter->copyMemberOut(
            &hostCellVec[0],
            MemoryLocation::HOST,
            reinterpret_cast<char*>(&hostBuffer[0]),
            MemoryLocation::HOST,
            40,
            memberPointer);

        for (std::size_t i = 0; i < hostBuffer.size(); ++i) {
            TS_ASSERT_EQUALS(i + 0.5, hostBuffer[i]);
        }

        for (std::size_t i = 0; i < hostBuffer.size(); ++i) {
            hostBuffer[i] = 47.11 + i;
        }

        filter->copyMemberIn(
            reinterpret_cast<char*>(&hostBuffer[0]),
            MemoryLocation::HOST,
            &hostCellVec[0],
            MemoryLocation::HOST,
            40,
            memberPointer);

        for (std::size_t i = 0; i < hostCellVec.size(); ++i) {
            TS_ASSERT_EQUALS(float(47.11 + i), hostCellVec[i].testValue);
        }
    }

    void testHostSoA()
    {
        std::vector<float> hostMemberVec(40);
        std::vector<float> hostBuffer(40, -1);

        for (std::size_t i = 0; i < hostMemberVec.size(); ++i) {
            hostMemberVec[i] = i + 0.7;
        }

        FilterBase<TestCell<2> > *filter = new DefaultFilter<TestCell<2>, float, float>();

        filter->copyStreakOut(
            reinterpret_cast<char*>(&hostMemberVec[0]),
            MemoryLocation::HOST,
            reinterpret_cast<char*>(&hostBuffer[0]),
            MemoryLocation::HOST,
            40,
            40);

        for (std::size_t i = 0; i < hostBuffer.size(); ++i) {
            TS_ASSERT_EQUALS(float(i + 0.7), hostBuffer[i]);
        }

        for (std::size_t i = 0; i < hostBuffer.size(); ++i) {
            hostBuffer[i] = 47.11 + i;
        }

        filter->copyStreakIn(
            reinterpret_cast<char*>(&hostBuffer[0]),
            MemoryLocation::HOST,
            reinterpret_cast<char*>(&hostMemberVec[0]),
            MemoryLocation::HOST,
            40,
            40);

        for (std::size_t i = 0; i < hostMemberVec.size(); ++i) {
            TS_ASSERT_EQUALS(float(47.11 + i), hostMemberVec[i]);
        }
    }
};

}
