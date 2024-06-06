#include <gtest/gtest.h>
#include "../hpool.hpp"

TEST(HPOOL, MANUAL_PTR_DISPOSE){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    auto ptr = pool.allocate();
    EXPECT_EQ(ptr._idx(), 0);
    EXPECT_EQ(pool.get_allocated_elements(), 1);

    ptr.free();
    EXPECT_EQ(pool.get_allocated_elements(), 0);
}

TEST(HPOOL, MANUAL_POOL_DISPOSE){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    auto ptr = pool.allocate();
    EXPECT_EQ(ptr._idx(), 0);
    EXPECT_EQ(pool.get_allocated_elements(), 1);

    pool.free(ptr);
    EXPECT_EQ(pool.get_allocated_elements(), 0);
}

TEST(HPOOL, AUTOMATIC_PTR_DISPOSE){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    {
        auto ptr = pool.allocate();
        EXPECT_EQ(ptr._idx(), 0);
        EXPECT_EQ(pool.get_allocated_elements(), 1);
    }

    ASSERT_EQ(pool.get_allocated_elements(), 0);
}

TEST(HPOOL, CORRECT_BLOCK_MANAGEMENT){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    auto ptr = pool.allocate();
    auto ptr1 = pool.allocate();
    EXPECT_EQ(ptr._idx(), 0);
    EXPECT_EQ(ptr1._idx(), 1);
    ASSERT_EQ(pool.get_nearest_free_block(), 2);

    ptr.free();
    ASSERT_EQ(pool.get_nearest_free_block(), 0);
}

TEST(HPOOL, USE_AFTER_FREE_BEHAVIOUR){
    hpool::HPool<int64_t> pool(32);
    ASSERT_EQ(pool.get_total_elements(), 32);

    auto ptr = pool.allocate();
    EXPECT_EQ(ptr._idx(), 0);
    EXPECT_EQ(pool.get_allocated_elements(), 1);

    ptr.free();
    EXPECT_EQ(pool.get_allocated_elements(), 0);
    ASSERT_ANY_THROW(auto t = *ptr);
}