#include <gtest/gtest.h>
#include <hpool.hpp>

class HPoolNoReallocationsTest : public ::testing::Test {
 protected:
	HPoolNoReallocationsTest() : pool_(10) {}

	hpool::HPool<int, hpool::ReallocationPolicy::NoReallocations> pool_;
};

TEST_F(HPoolNoReallocationsTest, ALLOCATE_AND_FREE) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	int* ptr1 = pool_.allocate();
	EXPECT_NE(ptr1, nullptr);
	EXPECT_EQ(pool_.allocated(), 1);

	int* ptr2 = pool_.allocate();
	EXPECT_NE(ptr2, nullptr);
	EXPECT_EQ(pool_.allocated(), 2);

	pool_.free(ptr1);
	EXPECT_EQ(pool_.allocated(), 1);

	pool_.free(ptr2);
	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolNoReallocationsTest, ALLOCATE_WHOLE_POOL) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	for (int i = 0; i < 10; ++i) {
		int* ptr = pool_.allocate();
		EXPECT_NE(ptr, nullptr);
	}

	EXPECT_EQ(pool_.allocated(), 10);

	// Allocate should return nullptr when pool is exhausted
	int* ptr = pool_.allocate();
	EXPECT_EQ(ptr, nullptr);
}

TEST_F(HPoolNoReallocationsTest, FREE_NULLPTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	pool_.free(nullptr);
	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolNoReallocationsTest, FREE_INVALID_PTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	int* ptr = new int;
	pool_.free(ptr);
	EXPECT_EQ(pool_.allocated(), 0);
	delete ptr;
}
