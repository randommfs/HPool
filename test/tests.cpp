#include <gtest/gtest.h>
#include <hpool.hpp>

using hpool::ReallocationPolicy::NoReallocations;
using hpool::ReallocationPolicy::OffsetRealloc;

class HPoolNoReallocationsTest : public ::testing::Test {
 protected:
	HPoolNoReallocationsTest() : pool_(10) {}

	hpool::HPool<int, NoReallocations> pool_;
};

class HPoolOffsetReallocTest : public ::testing::Test {
 protected:
	HPoolOffsetReallocTest() : pool_(10) {}

	hpool::HPool<int, OffsetRealloc> pool_;
};

TEST_F(HPoolNoReallocationsTest, ALLOCATE_AND_FREE) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	auto ptr1 = pool_.allocate();
	EXPECT_NE(ptr1, nullptr);
	EXPECT_EQ(pool_.allocated(), 1);

	auto ptr2 = pool_.allocate();
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

	hpool::Ptr<int, NoReallocations> prev_ptr;
	for (int i = 0; i < 10; ++i) {
		auto ptr = pool_.allocate();
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
	auto ptr = pool_.allocate();
	EXPECT_EQ(ptr, nullptr);
}

TEST_F(HPoolNoReallocationsTest, FREE_NULLPTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	hpool::Ptr<int, NoReallocations> null_ptr;

	pool_.free(null_ptr);
	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolNoReallocationsTest, MULTIPLE_POINTERS_VALIDATION) {
	std::array<hpool::Ptr<int, NoReallocations>, 10> pointers;
	
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

TEST_F(HPoolOffsetReallocTest, ALLOCATE_AND_FREE) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	auto ptr1 = pool_.allocate();
	EXPECT_NE(ptr1, nullptr);
	EXPECT_EQ(pool_.allocated(), 1);

	auto ptr2 = pool_.allocate();
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

	hpool::Ptr<int, OffsetRealloc> prev_ptr = nullptr;
	hpool::Ptr<int, OffsetRealloc> current_ptr = nullptr;
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
	auto ptr = pool_.allocate();
	EXPECT_EQ(pool_.size(), 20);
	EXPECT_EQ(pool_.allocated(), 11);
	EXPECT_NE(ptr, nullptr);
}

TEST_F(HPoolOffsetReallocTest, FREE_NULLPTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	hpool::Ptr<int, OffsetRealloc> null_ptr;

	pool_.free(null_ptr);

	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolOffsetReallocTest, MULTIPLE_POINTERS_VALIDATION) {
	std::array<hpool::Ptr<int, OffsetRealloc>, 20> pointers;
	
	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		pointers[i] = pool_.allocate();
		*pointers[i] = i; 
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

TEST_F(HPoolOffsetReallocTest, MULTIPLE_POINTERS_VALIDATION__NON_TRIVIALLY_COPYABLE) {
	std::array<hpool::Ptr<std::string, OffsetRealloc>, 20> pointers;
	hpool::HPool<std::string, hpool::ReallocationPolicy::OffsetRealloc> _pool{5};
	
	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		pointers[i] = _pool.allocate();
		*pointers[i] = std::to_string(i); 
	}

	// Validate values
	for (int i = 0; i < 20; ++i)
		EXPECT_EQ(*pointers[i], std::to_string(i));

	// Free and validate values
	for (int i = 19; i >= 0; --i) {
		_pool.free(pointers[i]);

		for (int j = i - 1; i >= 0; --i)
			EXPECT_EQ(*pointers[i], std::to_string(i));
	}
}

TEST_F(HPoolOffsetReallocTest, MULTIPLE_POINTERS_VALIDATION__STRING_VIEW) {
	std::array<hpool::Ptr<std::string_view, OffsetRealloc>, 20> pointers;
	hpool::HPool<std::string_view, hpool::ReallocationPolicy::OffsetRealloc> _pool{5};
	
	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		pointers[i] = _pool.allocate();
		*pointers[i] = std::to_string(i); 
	}

	// Validate values
	for (int i = 0; i < 20; ++i)
		EXPECT_EQ(*pointers[i], std::to_string(i));

	// Free and validate values
	for (int i = 19; i >= 0; --i) {
		_pool.free(pointers[i]);

		for (int j = i - 1; i >= 0; --i)
			EXPECT_EQ(*pointers[i], std::to_string(i));
	}
}
