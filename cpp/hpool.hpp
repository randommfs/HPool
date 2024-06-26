#pragma once

namespace hpool{
    template<typename T>
    class pool_ptr;

    template<typename T>
    struct Element{
        T object;
        bool used;
    };

    template<typename T>
    class IPool{
    public:
        virtual pool_ptr<T> allocate_pptr() = 0;
        virtual T* allocate() = 0;
        virtual void free(pool_ptr<T>&) = 0;
        virtual void free(T*) = 0;

        virtual bool is_valid() = 0;
        virtual uint32_t get_total_elements() = 0;
        virtual uint32_t get_allocated_elements() = 0;
        virtual uint32_t get_nearest_free_block() = 0;
    };

    template<typename T>
    class pool_ptr{
    private:
        Element<T>* ptr;
        IPool<T>& pool;
        uint32_t index;
        bool valid;
    public:
        pool_ptr(Element<T>* ptr, IPool<T>& pool, uint32_t index) :
            ptr(ptr),
            pool(pool),
            index(index),
            valid(true){}

        T& operator*();
        T* operator->();
        T* get();

        void free();

        uint32_t _idx();
        void _validate();
        bool is_valid();

        ~pool_ptr();
    };

    template<typename T>
    class HPool : IPool<T>{
    private:
        Element<T>* pool;
        uint32_t total_elements;
        uint32_t allocated_elements;
        uint32_t nearest_free_block;
        bool has_free_blocks;
        bool valid;

        uint32_t find_nearest_free_block();
    public:
        explicit HPool(uint32_t element_count) :
            total_elements(element_count),
            allocated_elements(0),
            nearest_free_block(0),
            has_free_blocks(true),
            valid(true){

            pool = static_cast<Element<T>*>(calloc(sizeof(Element<T>), element_count));
            if (!pool)
                throw std::runtime_error("Failed to allocate memory");
        }
        pool_ptr<T> allocate_pptr() override;
        T* allocate() override;
        void free(pool_ptr<T>&) override;
        void free(T*) override;

        bool is_valid() override;
        uint32_t get_total_elements() override;
        uint32_t get_allocated_elements() override;
        uint32_t get_nearest_free_block() override;

        ~HPool();
    };
}

template<typename T>
T& hpool::pool_ptr<T>::operator*() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return ptr->object;
}

template<typename T>
T* hpool::pool_ptr<T>::operator->() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return &ptr->object;
}

template<typename T>
T* hpool::pool_ptr<T>::get() {
    if (!ptr->used)
        throw std::runtime_error("use-after-free detected!");
    return &ptr->object;
}

template<typename T>
void hpool::pool_ptr<T>::free(){
    if (pool.is_valid())
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
    return valid;
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
hpool::pool_ptr<T> hpool::HPool<T>::allocate_pptr() {
    uint32_t index = nearest_free_block;
    pool[index].used = true;
    ++allocated_elements;
    nearest_free_block = find_nearest_free_block();
    return pool_ptr<T>(&pool[index], *this, index);
}

template<typename T>
T* hpool::HPool<T>::allocate() {
    uint32_t index = nearest_free_block;
    pool[index].used = true;
    ++allocated_elements;
    nearest_free_block = find_nearest_free_block();
    return &pool[index].object;
}

template<typename T>
void hpool::HPool<T>::free(hpool::pool_ptr<T>& ptr){
    pool[ptr._idx()].used = false;
    --allocated_elements;
    if (ptr._idx() < nearest_free_block)
        nearest_free_block = ptr._idx();
}

template<typename T>
void hpool::HPool<T>::free(T* ptr){
    uint32_t index = (reinterpret_cast<std::uintptr_t>(ptr) - reinterpret_cast<std::uintptr_t>(pool)) / sizeof(T) / 2;
    pool[index].used = false;
    --allocated_elements;
    if (index < nearest_free_block)
        nearest_free_block = index;
}

template<typename T>
uint32_t hpool::HPool<T>::get_total_elements() {
    return total_elements;
}

template<typename T>
uint32_t hpool::HPool<T>::get_allocated_elements() {
    return allocated_elements;
}

template<typename T>
uint32_t hpool::HPool<T>::get_nearest_free_block() {
    return nearest_free_block;
}

template<typename T>
bool hpool::HPool<T>::is_valid() {
    return valid;
}

template<typename T>
hpool::HPool<T>::~HPool(){
    std::free(pool);
    valid = false;
}
