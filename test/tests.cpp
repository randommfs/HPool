#include <gtest/gtest.h>
#include <new_hpool.hpp>
#include <string>

class HPoolTests : public ::testing::Test {
protected:
  HPoolTests() : pool_(10) {}

  HPool::HPool<int> pool_;
};

class HPoolMultiTypeTests : public ::testing::Test {
protected:
  HPoolMultiTypeTests() : pool_(10) {}

  HPool::HPool<int, std::string, double> pool_;
};

TEST_F(HPoolTests, ALLOCATE_AND_FREE) {
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

TEST_F(HPoolTests, ALLOCATE_WHOLE_POOL) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	HPool::Ptr<int> prev_ptr;
	HPool::Ptr<int> current_ptr;
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

TEST_F(HPoolTests, FREE_NULLPTR) {
	EXPECT_EQ(pool_.size(), 10);
	EXPECT_EQ(pool_.allocated(), 0);

	HPool::Ptr<int> null_ptr;

	pool_.free(null_ptr);

	EXPECT_EQ(pool_.allocated(), 0);
}

TEST_F(HPoolTests, MULTIPLE_POINTERS_VALIDATION) {
	std::array<HPool::Ptr<int>, 20> pointers;
	
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

		for (int j = i - 1; j >= 0; --j)
			EXPECT_EQ(*pointers[j], j);
	}
}

TEST_F(HPoolMultiTypeTests, MULTIPLE_POINTERS_VALIDATION__NON_TRIVIALLY_COPYABLE) {
	std::array<HPool::Ptr<int, std::variant<int, std::string, double>>, 20> int_ptrs;
  std::array<HPool::Ptr<std::string, std::variant<int, std::string, double>>, 20> str_ptrs;
  std::array<HPool::Ptr<double, std::variant<int, std::string, double>>, 20> dbl_ptrs;

	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		int_ptrs[i] = pool_.allocate<int>();
		*int_ptrs[i] = i;
	}

	// Validate values
	for (int i = 0; i < 20; ++i)
		EXPECT_EQ(*int_ptrs[i], std::to_string(i));

	// Free and validate values
	for (int i = 19; i >= 0; --i) {
		pool_.free(pointers[i]);

		for (int j = i - 1; j >= 0; --j)
			EXPECT_EQ(*pointers[j], std::to_string(j));
	}
}

/*
TEST_F(HPoolOffsetReallocTest, MULTIPLE_POINTERS_VALIDATION__STRING_VIEW) {
	std::array<hpool::Ptr<std::string_view, OffsetRealloc>, 20> pointers;
  std::array<std::string, 20> strings;
	hpool::HPool<std::string_view, hpool::ReallocationPolicy::OffsetRealloc> _pool{5};
	
	// Allocate memory
	for (int i = 0; i < 20; ++i) {
		pointers[i] = _pool.allocate();
    strings[i] = std::to_string(i);
		*pointers[i] = strings[i];
	}

	// Validate values
	for (int i = 0; i < 20; ++i)
		EXPECT_EQ(*pointers[i], std::to_string(i));

	// Free and validate values
	for (int i = 19; i >= 0; --i) {
		_pool.free(pointers[i]);

		for (int j = i - 1; j >= 0; --j)
			EXPECT_EQ(*pointers[j], std::to_string(j));
	}
}

TEST_F(HPoolOffsetReallocTest, CTOR_ARGS) {
  auto ptr = pool_.allocate(42);
  EXPECT_EQ(*ptr, 42);
}
*/