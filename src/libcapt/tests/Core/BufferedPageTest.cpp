#include "libcapt/Utility/BufferedPage.hpp"
#include "libcapt/Protocol/PageParams.hpp"
#include "MemoryStream.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ios>
#include <cstdint>
#include <utility>
#include <vector>

using namespace Capt;

TEST(BufferedPageTest, SeekPos) {
    const std::vector<uint8_t> buff = {1, 2, 3, 4, 5, 6, 7, 8};
    MemoryStream ms(buff);
    Utility::BufferedPage page(0, PageParams{}, ms.rdbuf());

    std::vector<char> temp(buff.size());
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), 0);
    ASSERT_EQ(page.pubseekpos(0), 0);

    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));
}

TEST(BufferedPageTest, SeekOff) {
    const std::vector<uint8_t> buff = {1, 2, 3, 4, 5, 6, 7, 8};
    MemoryStream ms(buff);
    Utility::BufferedPage page(0, PageParams{}, ms.rdbuf());

    std::vector<char> temp(buff.size());
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    ASSERT_EQ(page.pubseekoff(0, std::ios_base::beg), 0);
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    ASSERT_EQ(page.pubseekoff(-temp.size(), std::ios_base::cur), 0);
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    ASSERT_EQ(page.pubseekoff(-temp.size(), std::ios_base::end), 0);
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));
}

TEST(BufferedPageTest, MoveCtor) {
    const std::vector<uint8_t> buff = {1, 2, 3, 4, 5, 6, 7, 8};
    MemoryStream ms(buff);
    Utility::BufferedPage page(0, PageParams{}, ms.rdbuf());

    std::vector<char> temp(buff.size());
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    Utility::BufferedPage newpage(std::move(page));
    ASSERT_EQ(newpage.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));
}

TEST(BufferedPageTest, MoveAssign) {
    const std::vector<uint8_t> buff = {1, 2, 3, 4, 5, 6, 7, 8};
    MemoryStream ms(buff);
    Utility::BufferedPage page(0, PageParams{}, ms.rdbuf());

    std::vector<char> temp(buff.size());
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));

    Utility::BufferedPage newpage(123, PageParams{}, nullptr);
    newpage = std::move(page);
    ASSERT_EQ(newpage.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray(buff));
}

TEST(BufferedPageTest, PartRead) {
    const std::vector<uint8_t> buff = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    MemoryStream ms(buff);
    Utility::BufferedPage page(0, PageParams{}, ms.rdbuf());

    std::vector<char> temp(8);
    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), temp.size());
    ASSERT_THAT(temp, testing::ElementsAreArray({1, 2, 3, 4, 5, 6, 7, 8}));

    ASSERT_EQ(page.pubseekpos(0), 0);

    std::vector<char> full(buff.size());
    ASSERT_EQ(page.sgetn(full.data(), full.size()), full.size());
    ASSERT_THAT(full, testing::ElementsAreArray(buff));

    ASSERT_EQ(page.sgetn(temp.data(), temp.size()), 0);
}
