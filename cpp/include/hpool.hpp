namespace hpool{
    template<typename T>
    class pool_ptr;

    template<typename T>
    class IPool{
    public:
        virtual pool_ptr<T> allocate() = 0;
        virtual void free(pool_ptr<T>&) = 0;

        virtual uint32_t get_total_elements() = 0;
        virtual uint32_t get_allocated_elements() = 0;
    };

    template<typename T>
    class pool_ptr{
    private:
        struct Element{
            T object;
            bool used;
        };

        Element* ptr;
        IPool<T>& pool;
        uint32_t index;
        bool valid;
    public:
        pool_ptr(T* ptr, IPool<T>& pool, uint32_t index) :
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
        struct Element{
            T object;
            bool used;
        };

        Element* pool;
        uint32_t total_elements;
        uint32_t allocated_elements;
        uint32_t nearest_free_block;
        bool has_free_blocks;

        uint32_t find_nearest_free_block();
    public:
        explicit HPool(uint32_t element_count) :
            total_elements(element_count),
            allocated_elements(0),
            nearest_free_block(0),
            has_free_blocks(true){

            pool = static_cast<Element*>(calloc(sizeof(Element), element_count));
            if (!pool)
                throw std::runtime_error("Failed to allocate memory");
        }
        pool_ptr<T> allocate() override;
        void free(pool_ptr<T>&) override;

        uint32_t get_total_elements() override;
        uint32_t get_allocated_elements() override;
    };
}