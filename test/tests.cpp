#include "../hpool.hpp"
#include <gtest/gtest.h>

TEST(HPOOL, PTR_FREE) {
  hpool::HPool<int64_t> pool(32);
  ASSERT_EQ(pool.get_total_elements(), 32);

  auto ptr = pool.allocate();
  EXPECT_EQ(pool.get_allocated_elements(), 1);

  *ptr = 5;
  pool.free(ptr);
  EXPECT_EQ(pool.get_allocated_elements(), 0);
}

TEST(HPOOL, ELEMENT_SIZE) {
  ASSERT_EQ(sizeof(hpool::Element<size_t, sizeof(size_t)>), 9);
}

TEST(HPOOL, CUSTOM_SIZE) {
  ASSERT_EQ(sizeof(hpool::Element<size_t, 32>), 33);
}