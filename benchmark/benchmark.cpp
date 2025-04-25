#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/poolfwd.hpp>

#include <cstdlib>

#include <algorithm>
#include <random>

#include <hpool.hpp>
#include "hbench.hpp"

#define TEST_SIZE (65536 * 8)

struct Vector {
  double x, y, z;
};

size_t *arr1[TEST_SIZE];
hpool::Ptr<size_t, hpool::ReallocationPolicy::NoReallocations> arr2[TEST_SIZE];
hpool::Ptr<size_t, hpool::ReallocationPolicy::OffsetRealloc> arr3[TEST_SIZE];

Vector *v_arr1[TEST_SIZE];
hpool::Ptr<Vector, hpool::ReallocationPolicy::NoReallocations> v_arr2[TEST_SIZE];
hpool::Ptr<Vector, hpool::ReallocationPolicy::OffsetRealloc> v_arr3[TEST_SIZE];

void alloc_boost_linear(boost::object_pool<size_t>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr1[i] = boost_pool.malloc();
}

void free_boost_linear(boost::object_pool<size_t>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    boost_pool.free(arr1[i]);
}

void alloc_boost_random(boost::object_pool<size_t>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr1[i] = boost_pool.malloc();
}

void free_boost_random(boost::object_pool<size_t>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    boost_pool.free(arr1[i]);
}

void alloc_boost_linear_vector(boost::object_pool<Vector>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    v_arr1[i] = boost_pool.malloc();
}

void free_boost_linear_vector(boost::object_pool<Vector>& boost_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    boost_pool.free(v_arr1[i]);
}

void alloc_hpool_linear(hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr2[i] = hpool_pool.allocate();
}

void free_hpool_linear(hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr2[i]);
}

void alloc_hpool_random(hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr2[i] = hpool_pool.allocate();
}

void free_hpool_random(hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr2[i]);
}

void alloc_hpool_linear_vector(hpool::HPool<Vector, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    v_arr2[i] = hpool_pool.allocate();
}

void free_hpool_linear_vector(hpool::HPool<Vector, hpool::ReallocationPolicy::NoReallocations>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(v_arr2[i]);
}

void alloc_hpool_linear_realloc(hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr3[i] = hpool_pool.allocate();
}

void free_hpool_linear_realloc(hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr3[i]);
}

void alloc_hpool_random_realloc(hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr3[i] = hpool_pool.allocate();
}

void free_hpool_random_realloc(hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr3[i]);
}

void alloc_hpool_linear_vector_realloc(hpool::HPool<Vector, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    v_arr3[i] = hpool_pool.allocate();
}

void free_hpool_linear_vector_realloc(hpool::HPool<Vector, hpool::ReallocationPolicy::OffsetRealloc>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(v_arr3[i]);
}

int main() {
  std::random_device rd;
  std::mt19937 g(rd());

  {
    boost::object_pool<size_t> boost_pool{};
    BENCH(alloc_boost_linear(boost_pool));
    BENCH(free_boost_linear(boost_pool));
  }
  {
    boost::object_pool<size_t> boost_pool{};
    BENCH(alloc_boost_random(boost_pool));
    std::shuffle(std::begin(arr1), std::end(arr1), g);
    BENCH(free_boost_random(boost_pool));
  }
  {
    boost::object_pool<Vector> boost_pool{};
    BENCH(alloc_boost_linear_vector(boost_pool));
    BENCH(free_boost_linear_vector(boost_pool));
  }

  {
    hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations> hpool_pool{TEST_SIZE};
    BENCH(alloc_hpool_linear(hpool_pool));
    BENCH(free_hpool_linear(hpool_pool));
  }
  {
    hpool::HPool<size_t, hpool::ReallocationPolicy::NoReallocations> hpool_pool{TEST_SIZE};
    BENCH(alloc_hpool_random(hpool_pool));
    std::shuffle(std::begin(arr1), std::end(arr1), g);
    BENCH(free_hpool_random(hpool_pool));
  }
  {
    hpool::HPool<Vector, hpool::ReallocationPolicy::NoReallocations> hpool_pool{TEST_SIZE};
    BENCH(alloc_hpool_linear_vector(hpool_pool));
    BENCH(free_hpool_linear_vector(hpool_pool));
  }

  {
    hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc> hpool_pool{2};
    BENCH(alloc_hpool_linear_realloc(hpool_pool));
    BENCH(free_hpool_linear_realloc(hpool_pool));
  }
  {
    hpool::HPool<size_t, hpool::ReallocationPolicy::OffsetRealloc> hpool_pool{2};
    BENCH(alloc_hpool_random_realloc(hpool_pool));
    std::shuffle(std::begin(arr1), std::end(arr1), g);
    BENCH(free_hpool_random_realloc(hpool_pool));
  }
  {
    hpool::HPool<Vector, hpool::ReallocationPolicy::OffsetRealloc> hpool_pool{2};
    BENCH(alloc_hpool_linear_vector_realloc(hpool_pool));
    BENCH(free_hpool_linear_vector_realloc(hpool_pool));
  }
}
