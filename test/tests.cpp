#include <memory>
#include <cstdint>

#include <hpool.hpp>
#include <gtest/gtest.h>

using namespace hpool;


TEST(HPool, AllocateAndFreePointer) {
	hpool::HPool<std::int64_t> pool(32);
	ASSERT_EQ(pool.size(), 32);

	auto ptr = pool.allocate();
	EXPECT_EQ(pool.allocated(), 1);

	pool.free(ptr);
	EXPECT_EQ(pool.allocated(), 0);
}


TEST(HPool, AllocateAndFreeMultipleTimes) {
	constexpr int poolSize = 3;

	hpool::HPool<std::int64_t> pool(poolSize);
	std::int64_t* ptrs[poolSize];

	for (int i = 0; i < poolSize; ++i) {
		ptrs[i] = pool.allocate();
	}

	EXPECT_EQ(pool.size(), pool.allocated());
	EXPECT_EQ(pool.allocate(), nullptr);

	for (int i = 0; i < poolSize - 1; ++i) {
		pool.free(ptrs[i]);
	}	

	EXPECT_EQ(pool.allocated(), 1);

	std::construct_at(ptrs[poolSize - 1], static_cast<std::int64_t>(0));
	EXPECT_EQ(*ptrs[poolSize - 1], 0);
}