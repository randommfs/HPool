#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

namespace hpool {

template <typename T, size_t BlockSize> struct Element {
  char object[BlockSize];
  bool used;
} __attribute__((packed));

template <typename T, size_t BlockSize = sizeof(T)> class HPool {
private:
  Element<T, sizeof(T)> *pool;
  uint32_t total_elements;
  uint32_t allocated_elements;
  uint32_t nearest_free_block;
  bool has_free_blocks;

  uint32_t find_nearest_free_block();

public:
  explicit HPool(uint32_t element_count)
      : total_elements(element_count), allocated_elements(0),
        nearest_free_block(0), has_free_blocks(true) {

    pool = static_cast<Element<T, sizeof(T)> *>(
        std::calloc(sizeof(Element<T, sizeof(T)>), element_count));
    if (!pool)
      throw std::runtime_error("Failed to allocate memory");
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
  if (nearest_free_block == -1)
    return nullptr;
  uint32_t index = nearest_free_block;
  pool[index].used = true;
  ++allocated_elements;
  nearest_free_block = find_nearest_free_block();
  return reinterpret_cast<T*>(&pool[index].object);
}

template <typename T, size_t BlockSize>
void hpool::HPool<T, BlockSize>::free(T *ptr) {
  assert(ptr);
  uint32_t index = (reinterpret_cast<std::uintptr_t>(ptr) -
                    reinterpret_cast<std::uintptr_t>(pool)) /
                   sizeof(Element<T, sizeof(T)>);
  assert(pool[index].used);
  pool[index].used = false;
  --allocated_elements;
  if (index < nearest_free_block)
    nearest_free_block = index;
}

template <typename T, size_t BlockSize>
uint32_t hpool::HPool<T, BlockSize>::get_total_elements() {
  return total_elements;
}

template <typename T, size_t BlockSize>
uint32_t hpool::HPool<T, BlockSize>::get_allocated_elements() {
  return allocated_elements;
}

template <typename T, size_t BlockSize>
uint32_t hpool::HPool<T, BlockSize>::find_nearest_free_block() {
  for (uint32_t i = nearest_free_block; i < total_elements; ++i) {
    if (!pool[i].used)
      return i;
  }
  return -1;
}

#pragma GCC diagnostic pop