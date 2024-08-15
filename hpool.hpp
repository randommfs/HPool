#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include <stdexcept>

namespace hpool {

template <typename T, size_t BlockSize> struct Element {
  char object[BlockSize];
  uint32_t ptr;
};

template <typename T, size_t BlockSize = sizeof(T)> class HPool {
private:
  Element<T, sizeof(T)> *pool;
  uint32_t total_elements;
  uint32_t allocated_elements;
  uint32_t free_ptr;
  bool has_free_blocks : 1;

public:
  explicit HPool(uint32_t element_count)
      : total_elements(element_count), allocated_elements(0),
        free_ptr(0), has_free_blocks(true) {

    pool = static_cast<Element<T, sizeof(T)> *>(
        std::calloc(sizeof(Element<T, sizeof(T)>), element_count));
    if (!pool)
      throw std::runtime_error("Failed to allocate memory");
    // Initialize pool
    for (uint32_t i = 0; i < element_count; ++i) {
      pool[i].ptr = i + 1;
    }
    pool[element_count - 1].ptr = element_count - 1;
  }
  T *allocate();
  void free(T *);

  uint32_t get_total_elements();
  uint32_t get_allocated_elements();

  ~HPool() { std::free(pool); }
};
} // namespace hpool

template <typename T, size_t BlockSize>
T *hpool::HPool<T, BlockSize>::allocate() {
  if (!has_free_blocks)
    return nullptr;
  uint32_t index = free_ptr;
  free_ptr = pool[index].ptr;
  ++allocated_elements;
  if (allocated_elements == total_elements)
    has_free_blocks = false;
  return reinterpret_cast<T*>(&pool[index].object);
}

template <typename T, size_t BlockSize>
void hpool::HPool<T, BlockSize>::free(T *ptr) {
  assert(ptr);
  uint32_t index = (reinterpret_cast<std::uintptr_t>(ptr) -
                    reinterpret_cast<std::uintptr_t>(pool)) /
                   sizeof(Element<T, sizeof(T)>);
  pool[index].ptr = free_ptr;
  free_ptr = index;
  --allocated_elements;
}

template <typename T, size_t BlockSize>
uint32_t hpool::HPool<T, BlockSize>::get_total_elements() {
  return total_elements;
}

template <typename T, size_t BlockSize>
uint32_t hpool::HPool<T, BlockSize>::get_allocated_elements() {
  return allocated_elements;
}
