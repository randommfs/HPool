#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/poolfwd.hpp>

#include <cstdlib>

#include <algorithm>
#include <random>

#include <hpool.hpp>
#include "hbench.hpp"

#define TEST_SIZE (65536 * 8)

size_t *arr1[TEST_SIZE];
size_t *arr2[TEST_SIZE];

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

void alloc_hpool_linear(hpool::HPool<size_t>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr2[i] = hpool_pool.allocate();
}

void free_hpool_linear(hpool::HPool<size_t>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr2[i]);
}

void alloc_hpool_random(hpool::HPool<size_t>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    arr2[i] = hpool_pool.allocate();
}

void free_hpool_random(hpool::HPool<size_t>& hpool_pool) {
  for (int i = 0; i < TEST_SIZE; ++i)
    hpool_pool.free(arr2[i]);
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
    hpool::HPool<size_t> hpool_pool{TEST_SIZE};
    BENCH(alloc_hpool_linear(hpool_pool));
    BENCH(free_hpool_linear(hpool_pool));
  }

  {
    hpool::HPool<size_t> hpool_pool{TEST_SIZE};
    BENCH(alloc_hpool_random(hpool_pool));
    std::shuffle(std::begin(arr2), std::end(arr2), g);
    BENCH(free_hpool_random(hpool_pool));
  }
}