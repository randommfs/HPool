#pragma once

#include <cstdint>
#include <cstdlib>

#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

namespace hpool {

template <typename T> struct Element {
  T object;
  bool used;
} __attribute__((packed));

template <typename T> class HPool {
private:
  Element<T> *pool;
  uint32_t total_elements;
  uint32_t allocated_elements;
  uint32_t nearest_free_block;
  bool has_free_blocks;

  uint32_t find_nearest_free_block();

public:
  explicit HPool(uint32_t element_count)
      : total_elements(element_count), allocated_elements(0),
        nearest_free_block(0), has_free_blocks(true) {

    pool = static_cast<Element<T> *>(
        std::calloc(sizeof(Element<T>), element_count));
    if (!pool)
      throw std::runtime_error("Failed to allocate memory");
  }
  T *allocate();
  void free(T *);

  uint32_t get_total_elements();
  uint32_t get_allocated_elements();

  ~HPool() {
    std::free(pool);
  }
};
} // namespace hpool

template <typename T>
T *hpool::HPool<T>::allocate() {
  uint32_t index = nearest_free_block;
  pool[index].used = true;
  ++allocated_elements;
  nearest_free_block = find_nearest_free_block();
  return &pool[index].object;
}

template <typename T> void hpool::HPool<T>::free(T *ptr) {
  uint32_t index = (reinterpret_cast<std::uintptr_t>(ptr) -
                    reinterpret_cast<std::uintptr_t>(pool)) /
                   sizeof(Element<T>);
  pool[index].used = false;
  --allocated_elements;
  if (index < nearest_free_block)
    nearest_free_block = index;
}

template <typename T> uint32_t hpool::HPool<T>::get_total_elements() {
  return total_elements;
}

template <typename T> uint32_t hpool::HPool<T>::get_allocated_elements() {
  return allocated_elements;
}

template<typename T> uint32_t hpool::HPool<T>::find_nearest_free_block() {
  for (uint32_t i = nearest_free_block; i < total_elements; ++i) {
    if (!pool[i].used) return i;
  }
  return -1;
}

#pragma GCC diagnostic pop