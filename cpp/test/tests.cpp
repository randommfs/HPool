#include <gtest/gtest.h>
#include <hpool.hpp>

TEST(HPOOL, MANUAL_PTR_DISPOSE){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    auto ptr = pool.allocate();
    EXPECT_EQ(ptr._idx(), 0);
    EXPECT_EQ(pool.get_allocated_elements(), 1);

    ptr.free();
    EXPECT_EQ(pool.get_allocated_elements(), 0);
}