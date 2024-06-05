#include <stdexcept>
#include <cstdint>
#include <cstdlib>

#include <hpool.hpp>

template<typename T>
T& hpool::pool_ptr<T>::operator*() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return *ptr;
}

template<typename T>
T* hpool::pool_ptr<T>::operator->() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return ptr;
}

template<typename T>
T* hpool::pool_ptr<T>::get() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return ptr;
}

template<typename T>
void hpool::pool_ptr<T>::free(){
    pool.free(*this);
}

template<typename T>
uint32_t hpool::pool_ptr<T>::_idx() {
    return index;
}

template<typename T>
void hpool::pool_ptr<T>::_validate() {
    valid = ptr->used;
}

template<typename T>
bool hpool::pool_ptr<T>::is_valid() {
    return ptr->used;
}

template<typename T>
hpool::pool_ptr<T>::~pool_ptr(){
    pool.free(*this);
}

template<typename T>
uint32_t hpool::HPool<T>::find_nearest_free_block() {
    for (uint32_t i = 0; i < total_elements; ++i)
        if (!pool[i].used) return i;
    has_free_blocks = false;
    return 0;
}

template<typename T>
hpool::pool_ptr<T> hpool::HPool<T>::allocate() {
    uint32_t index = nearest_free_block;
    pool[index].used = true;
    nearest_free_block = find_nearest_free_block();
    return pool_ptr<T>(&pool[index].object, *this, index);
}

template<typename T>
void hpool::HPool<T>::free(hpool::pool_ptr<T>& ptr){
    pool[ptr._idx()].used = false;
    --allocated_elements;
    if (ptr._idx() < nearest_free_block)
        nearest_free_block = ptr._idx();
}

template<typename T>
uint32_t hpool::HPool<T>::get_total_elements() {
    return total_elements;
}

template<typename T>
uint32_t hpool::HPool<T>::get_allocated_elements() {
    return allocated_elements;
}