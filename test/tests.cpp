#include <gtest/gtest.h>
#include <hpool.hpp>

class HPoolNoReallocationsTest : public ::testing::Test {
 protected:
	HPoolNoReallocationsTest() : pool_(10) {}

	hpool::HPool<int, hpool::ReallocationPolicy::NoReallocations> pool_;
};

class HPoolOffsetReallocTest : public ::testing::Test {
 protected:
	HPoolOffsetReallocTest() : pool_(10) {}

	hpool::HPool<int, hpool::ReallocationPolicy::OffsetRealloc> pool_;
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

	int* prev_ptr = nullptr;
	for (int i = 0; i < 10; ++i) {
		int* ptr = pool_.allocate();
		EXPECT_NE(ptr, nullptr);
		EXPECT_NE(ptr, prev_ptr);
		if (prev_ptr){
			EXPECT_NE(*prev_ptr, *ptr - 1);
			std::cout << *prev_ptr << '\n';
		}
			
		*ptr = i;
		prev_ptr = ptr;
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

TEST_F(HPoolNoReallocationsTest, MULTIPLE_POINTERS_VALIDATION) {
	std::array<int*, 10> pointers;
	
	// Allocate memory
	for (int i = 0; i < 10; ++i) {
		pointers[i] = pool_.allocate();
		*pointers[i] = i; 
	}

	// Validate values
	for (int i = 0; i < 10; ++i)
		EXPECT_EQ(*pointers[i], i);

	// Free and validate values
	for (int i = 9; i >= 0; --i) {
		pool_.free(pointers[i]);

		for (int j = i - 1; i >= 0; --i)
			EXPECT_EQ(*pointers[i], i);
	}
}

TEST_F(HPoolNoReallocationsTest, UNIQUE_PTR) {
	{
		auto ptr = hpool::make_unique(pool_, 1);
		EXPECT_EQ(pool_.allocated(), 1);
	}
	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolOffsetReallocTest, ALLOCATE_AND_FREE) {
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

TEST_F(HPoolOffsetReallocTest, ALLOCATE_WHOLE_POOL) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	int* prev_ptr = nullptr;
	int* current_ptr = nullptr;
	for (int i = 0; i < 10; ++i) {
		current_ptr = pool_.allocate();
		EXPECT_NE(current_ptr, nullptr);
		EXPECT_NE(current_ptr, prev_ptr);
			
		*current_ptr = i;
		prev_ptr = current_ptr;
	}
	EXPECT_EQ(*current_ptr, *prev_ptr);
	EXPECT_EQ(pool_.allocated(), 10);

	// Pool should reallocate memory on allocation if storage is exhausted
	int* ptr = pool_.allocate();
	EXPECT_EQ(pool_.size(), 20);
	EXPECT_EQ(pool_.allocated(), 11);
	EXPECT_NE(ptr, nullptr);
}

TEST_F(HPoolOffsetReallocTest, FREE_NULLPTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	pool_.free(nullptr);
	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolOffsetReallocTest, FREE_INVALID_PTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	int* ptr = new int;
	pool_.free(ptr);
	EXPECT_EQ(pool_.allocated(), 0);
	delete ptr;
}

TEST_F(HPoolOffsetReallocTest, MULTIPLE_POINTERS_VALIDATION) {
	std::array<int*, 20> pointers;
	
	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		pointers[i] = pool_.allocate();
		*pointers[i] = i; 
		std::cout << i << '\n';
	}

	// Validate values
	for (int i = 0; i < 20; ++i)
		EXPECT_EQ(*pointers[i], i);

	// Free and validate values
	for (int i = 19; i >= 0; --i) {
		pool_.free(pointers[i]);

		for (int j = i - 1; i >= 0; --i)
			EXPECT_EQ(*pointers[i], i);
	}
}

TEST_F(HPoolOffsetReallocTest, UNIQUE_PTR) {
	{
		auto ptr = hpool::make_unique(pool_, 1);
		EXPECT_EQ(pool_.allocated(), 1);
	}
	EXPECT_EQ(pool_.allocated(), 0);
}