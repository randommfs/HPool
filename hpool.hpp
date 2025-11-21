#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace HPool {
  namespace __hpool_impl {
    template<typename T1>
    class IPointer;

    template<typename T, typename PointerImpl>
    class IHPool {
    protected:
      struct HPoolElemWrapper {
        T value;
        std::uint32_t next;
      };

      friend class IPointer<T>;
      std::unique_ptr<HPoolElemWrapper[]> m_pool;
      std::uint32_t m_totalsize;
      std::uint32_t m_allocated;
      std::uint32_t m_next;
      std::ptrdiff_t m_offset;
      bool m_bad;

      HPoolElemWrapper* ParseAt(std::uint32_t hint, HPoolElemWrapper* pl) noexcept {
        auto* pool = pl ? pl : m_pool.get();
        return pool + hint;
      }

      T* ParseNext() noexcept {
        auto block = ParseAt(m_next);
        m_next = block->next;
        ++m_allocated;
        return block->value;
      }

      T* AddOffset(T* ptr) {
        return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr + m_offset));
      }

      T* SubtractOffset(T* ptr) {
        return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr - m_offset));
      }

    public:
      IHPool(std::uint32_t size)
        : m_pool(std::make_unique<HPoolElemWrapper[]>(size))
        , m_totalsize(size)
        , m_allocated(0)
        , m_next(0)
        , m_bad(false) {

        if (!m_pool) {
          m_bad = true;
        }

        for (std::uint32_t i = 0; i < m_totalsize; ++i) {
          auto blk = ParseAt(i);
          blk->next = i + 1;
        }
        auto last = ParseAt(m_totalsize - 1);
        last->next = m_totalsize - 1;
      }

      std::uint32_t size() const noexcept {
        return m_totalsize;
      }
      std::uint32_t allocated() const noexcept {
        return m_allocated;
      }

      virtual ~IHPool() { }
    };

    template<typename T, typename... Args>
    class IDeleter {
    public:
      IDeleter() = default;
      IDeleter([[maybe_unused]] const IDeleter<T>& other) { }
      IDeleter([[maybe_unused]] IDeleter<T>&& other) { }

      void operator()(T* ptr) const noexcept;
    };

    template<typename T>
    class IPointer {
    public:
      IPointer() = default;
      IPointer(const std::nullptr_t) : m_ptr(nullptr) { }
      IPointer(const IPointer<T>& other) : m_ptr(other.m_ptr) { }
      IPointer<T>& operator=(IPointer<T> other) {
        m_ptr = other.m_ptr;
        return *this;
      }

      T& operator*() { return *m_ptr; }
      T& operator->() { return *m_ptr; }
      T* operator+() { return m_ptr; }
      T* operator-() { return m_ptr; }
      bool operator==(const std::nullptr_t) const { return m_ptr == nullptr; }
      bool operator!=(const std::nullptr_t) const { return m_ptr != nullptr; }
      bool operator==(IPointer<T> ptr) const { return m_ptr == ptr.m_ptr; }
      bool operator!=(IPointer<T> ptr) const { return m_ptr != ptr.m_ptr; }
      explicit operator bool() { return m_ptr != nullptr; }
    protected:
      T* m_ptr;
    };
  }

  template<typename T>
  class Ptr;

  template<typename... Ts>
  class HPool;

  template<typename T>
  struct is_variant;

  template<typename T>
  struct is_variant : std::false_type { };

  template<typename... T>
  struct is_variant<std::variant<T...>> : std::true_type { };

  template<typename T1, typename T2, typename... Ts>
  class HPool<T1, T2, Ts...> : public __hpool_impl::IHPool<std::variant<T1, T2, Ts...>, Ptr<std::variant<T1, T2, Ts...>>> {
    using Type = std::variant<T1, T2, Ts...>;
    using PointerType = Ptr<Type>;
    using Base = __hpool_impl::IHPool<Type, PointerType>;
  public:
    explicit HPool(std::uint32_t size)
      : Base(size) { }

    template<typename T, typename... Args>
    PointerType allocate(Args... args);

    template<typename T, typename... Args>
    PointerType emplace(Args... args);
  };

  template<typename T>
  class HPool<T> : public __hpool_impl::IHPool<T, Ptr<T>> {
  public:
    explicit HPool(std::uint32_t);

    template<typename... Args>
    Ptr<T> allocate(Args... args) noexcept;
    void free(Ptr<T>&) noexcept;
  private:
    using ElemPtr = __hpool_impl::IHPool<T, Ptr<T>>::template HPoolElemWrapper<T>*;
    std::ptrdiff_t m_offset;

    ElemPtr AddOffset();
  };

  template<typename T>
  class Ptr : public __hpool_impl::IPointer<T> {

  };
}

template<typename T1, typename T2, typename... Ts>
template<typename T, typename... Args>
HPool::Ptr<std::variant<T1, T2, Ts...>>
HPool::HPool<T1, T2, Ts...>::allocate(Args... args) {
  if (Base::m_allocated == Base::m_totalsize) {
    std::size_t new_size = Base::m_totalsize * 2;
    auto ptr = std::make_unique<Type[]>(sizeof(Type) * new_size);
    if (!ptr) {
      return nullptr;
    }

    std::uint32_t orig_size = Base::m_totalsize;

    for (std::uint32_t i = 0; i < orig_size; ++i) {
      auto blk = Base::ParseAt(i);
      auto* src = reinterpret_cast<Type*>(Base::ParseAt(i));
      auto* dst = reinterpret_cast<Type*>(Base::ParseAt(i, ptr.get()));
      std::construct_at(dst, std::move(*src));
      std::destroy_at(src);
    }

    std::ptrdiff_t delta = reinterpret_cast<std::uint8_t*>(ptr.get()) - reinterpret_cast<std::uint8_t*>(Base::m_pool.get());
    Base::m_offset += delta;
    Base::m_totalsize = new_size;
    Base::m_pool.swap(ptr);
    Base::m_next = orig_size;

    for (std::uint32_t i = orig_size; i < new_size; ++i) {
      auto* block = Base::ParseAt(i);
      block->next = i + 1;
    }
    auto* last = Base::ParseAt(new_size - 1);
    last->next = new_size - 1;
  }

  auto ptr = Base::ParseNext();
  std::construct_at(ptr, T{std::forward<Args>(args)...});
  return PointerType(Base::SubtractOffset(ptr), *this);
}



namespace __hpool {

	/* All policies */
	enum ReallocationPolicy {
		NoReallocations,
		OffsetRealloc
	};

	namespace _internal {
		template<typename T, ReallocationPolicy Policy>
		class IPointer;

		/* Generic pool interface. Holds generic function implementations */
		template<typename T, ReallocationPolicy Policy>
		class IHPool {
		protected:
			friend class IPointer<T, Policy>;
			std::unique_ptr<char[]> pool_;
			std::uint32_t totalSize_;
			std::uint32_t allocatedSize_;
			std::uint32_t next_;

			std::pair<std::uint32_t&, T*> parseAt(std::uint32_t hint, char* ptr = nullptr) noexcept;
			T* parseNext() noexcept;
		public:
			IHPool(std::uint32_t size)
				: pool_(std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t)) * size))
				, totalSize_(size)
				, allocatedSize_(0)
				, next_(0)
			{
				if (!pool_) {
					throw std::runtime_error("failed to allocate enough memory");
				}

				for (std::uint32_t i = 0; i < totalSize_; ++i) {
					auto block = parseAt(i);
					block.first = i + 1;
				}
				auto last = parseAt(totalSize_ - 1);
				last.first = totalSize_ - 1;

			}

			/* Implemented in interface - doesn't depend on derived classes */
			std::uint32_t size() const noexcept;
			std::uint32_t allocated() const noexcept;

			virtual ~IHPool() { }
		};

		template<typename T, typename... Args>
		class IDeleter {
		public:
			IDeleter() = default;
			IDeleter([[maybe_unused]] const IDeleter<T>& other) { }
			IDeleter([[maybe_unused]] IDeleter<T>&& other) { }

			void operator()(T* ptr) const noexcept;
		};

		template<typename T, ReallocationPolicy Policy>
		class IPointer {
		private:
			friend class IHPool<T, Policy>;
		public:
			virtual T& operator*() = 0;
			virtual T& operator->() = 0;
			virtual T* operator+() = 0;
			virtual T* operator-() = 0;
			virtual operator bool() = 0;
		};
	}

	template<typename T, ReallocationPolicy Policy>
	class HPool;
	template<typename T, ReallocationPolicy Policy>
	class Ptr;

	template<typename T>
	class HPool<T, ReallocationPolicy::NoReallocations> : public _internal::IHPool<T, ReallocationPolicy::NoReallocations> {
	public:
		explicit HPool(std::uint32_t);
    
    template<typename... Args>
		Ptr<T, ReallocationPolicy::NoReallocations> allocate(Args&&... args) noexcept;
		void free(Ptr<T, ReallocationPolicy::NoReallocations>&) noexcept;
	};

	template<typename T>
	class Ptr<T, ReallocationPolicy::NoReallocations> : public _internal::IPointer<T, ReallocationPolicy::NoReallocations> {
	private:
		friend class HPool<T, ReallocationPolicy::NoReallocations>;
		T* ptr_;
		Ptr(T* ptr) : ptr_(ptr) { }
	public:
		Ptr() = default;
		Ptr(const std::nullptr_t) : ptr_(nullptr) { }
		Ptr(const Ptr<T, ReallocationPolicy::NoReallocations>& other) {
			ptr_ = other.ptr_;
		}
		Ptr<T, ReallocationPolicy::NoReallocations>& operator=(Ptr<T, ReallocationPolicy::NoReallocations> other) {
			ptr_ = other.ptr_;
			return *this;
		}
		T& operator*() override { return *ptr_; }
		T& operator->() override { return *ptr_; }
		T* operator+() override { return ptr_; }
		T* operator-() override { return ptr_; }
		bool operator==(const std::nullptr_t) const { return ptr_ == nullptr; }
		bool operator!=(const std::nullptr_t) const { return ptr_ != nullptr; }
		bool operator==(Ptr<T, ReallocationPolicy::NoReallocations> ptr) const { return ptr_ == ptr.ptr_; }
		bool operator!=(Ptr<T, ReallocationPolicy::NoReallocations> ptr) const { return ptr_ != ptr.ptr_; }
		explicit operator bool() override { return ptr_ != nullptr; }
	};


	template<typename T>
	class HPool<T, ReallocationPolicy::OffsetRealloc> final : public _internal::IHPool<T, ReallocationPolicy::OffsetRealloc> {
	private:
		friend class Ptr<T, ReallocationPolicy::OffsetRealloc>;
		std::ptrdiff_t offset_;

		T* addOffset(T*);
		T* subtractOffset(T*);
	public:
		explicit HPool(std::uint32_t);

    template<typename... Args>
		Ptr<T, ReallocationPolicy::OffsetRealloc> allocate(Args&&... args) noexcept;
		void free(Ptr<T, ReallocationPolicy::OffsetRealloc>&) noexcept;
	};

	template<typename T>
	class Ptr<T, ReallocationPolicy::OffsetRealloc> : public _internal::IPointer<T, ReallocationPolicy::OffsetRealloc> {
	private:
		friend class HPool<T, ReallocationPolicy::OffsetRealloc>;
		HPool<T, ReallocationPolicy::OffsetRealloc>* pool_;
		T* ptr_;
		Ptr(T* ptr, HPool<T, ReallocationPolicy::OffsetRealloc>& pool) : pool_(&pool), ptr_(ptr)  { }
	public:
		Ptr() = default;
		Ptr(const std::nullptr_t) : pool_(nullptr), ptr_(nullptr) { }
		Ptr(HPool<T, ReallocationPolicy::OffsetRealloc>& pool) : pool_(&pool), ptr_(nullptr) { }
		Ptr(const Ptr<T, ReallocationPolicy::OffsetRealloc>& other) {
			pool_ = other.pool_;
			ptr_ = other.ptr_;
		}
		Ptr<T, ReallocationPolicy::OffsetRealloc>& operator=(Ptr<T, ReallocationPolicy::OffsetRealloc> other) {
			pool_ = other.pool_;
			ptr_ = other.ptr_;
			return *this;
		}
		T& operator*() override { return *pool_->addOffset(ptr_); }
		T& operator->() override { return *pool_->addOffset(ptr_); }
		T* operator+() override { return pool_->addOffset(ptr_); }
		T* operator-() override { return pool_->addOffset(ptr_); }
		bool operator==(const std::nullptr_t) const { return ptr_ == nullptr; }
		bool operator!=(const std::nullptr_t) const { return ptr_ != nullptr; }
		bool operator==(Ptr<T, ReallocationPolicy::OffsetRealloc> ptr) const { return ptr_ == ptr.ptr_; }
		bool operator!=(Ptr<T, ReallocationPolicy::OffsetRealloc> ptr) const { return ptr_ != ptr.ptr_; }
		explicit operator bool() override { return ptr_ != nullptr; }
	};

	template<typename T, ReallocationPolicy Policy, typename Enable = void>
	class Deleter;

	template<typename T,ReallocationPolicy ReallocPolicy>
	class Deleter<T, ReallocPolicy> : private _internal::IDeleter<T> {
	private:
		HPool<T, ReallocPolicy>* pool_;
	public:
		Deleter(HPool<T, ReallocPolicy>& pool);

		void operator()(T* ptr) const noexcept;
	};

} // namespace hpol

// Generic implementations
template <typename T, hpool::ReallocationPolicy Policy> 
std::uint32_t hpool::_internal::IHPool<T, Policy>::size() const noexcept {
	return totalSize_;
}

template <typename T, hpool::ReallocationPolicy Policy> 
std::uint32_t hpool::_internal::IHPool<T, Policy>::allocated() const noexcept {
	return allocatedSize_;
}

template <typename T, hpool::ReallocationPolicy Policy> 
std::pair<std::uint32_t&, T*> hpool::_internal::IHPool<T, Policy>::parseAt(std::uint32_t hint, char* pl) noexcept {
	char* pool = pl ? pl : this->pool_.get();
	char* shift = reinterpret_cast<char*>(pool + hint * (sizeof(T) + sizeof(std::uint32_t)));
	return std::make_pair(
		std::ref(reinterpret_cast<std::uint32_t&>(*shift)),
		reinterpret_cast<T*>(shift + sizeof(std::uint32_t))
	);
}

template <typename T, hpool::ReallocationPolicy Policy> 
T* hpool::_internal::IHPool<T, Policy>::parseNext() noexcept {
	auto block = parseAt(this->next_);

	this->next_ = block.first;
	++this->allocatedSize_;
	return block.second;
}

// HPool NoReallocations implementation
template<typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::HPool(std::uint32_t size)
	: hpool::_internal::IHPool<T, hpool::ReallocationPolicy::NoReallocations>(size) { }

template <typename T>
template <typename... Args>
hpool::Ptr<T, hpool::ReallocationPolicy::NoReallocations> hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::allocate(Args&&... args) noexcept {
	if (this->allocatedSize_ == this->totalSize_) {
		return nullptr;
	}
  
  auto ptr = this->parseNext();
  std::construct_at(ptr, std::forward<Args>(args)...);
	return Ptr<T, NoReallocations>(ptr);
}

template <typename T> 
void hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::free(hpool::Ptr<T, hpool::ReallocationPolicy::NoReallocations>& ptr) noexcept {
        if (!ptr)
                return;

        std::ptrdiff_t shift = reinterpret_cast<std::ptrdiff_t>(ptr.ptr_) - 
                reinterpret_cast<std::ptrdiff_t>(this->pool_.get());
        // Correct division by block size
        shift /= (sizeof(T) + sizeof(std::uint32_t)); // Fix here

        if (shift < 0 || static_cast<std::uint32_t>(shift) >= this->totalSize_) {
                return;
        }

        auto block = this->parseAt(shift);
        block.first = this->next_;
        this->next_ = shift;

        --this->allocatedSize_;
}

// Deleter implementation
template<typename T,hpool::ReallocationPolicy ReallocPolicy>
hpool::Deleter<T, ReallocPolicy>::Deleter(hpool::HPool<T, ReallocPolicy>& pool)
	: pool_(&pool)
{}

template<typename T,hpool::ReallocationPolicy ReallocPolicy>
void hpool::Deleter<T, ReallocPolicy>::operator()(T* ptr) const noexcept {
	pool_->free(ptr);
}

// HPool OffsetRealloc implementation
template<typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::HPool(std::uint32_t size)
	: hpool::_internal::IHPool<T, hpool::ReallocationPolicy::OffsetRealloc>(size)
	, offset_(0) { }

template<typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::addOffset(T* ptr) {
	return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) + offset_);
} 

template<typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::subtractOffset(T* ptr) {
	return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) - offset_);
}

template <typename T>
template <typename... Args>
hpool::Ptr<T, hpool::ReallocationPolicy::OffsetRealloc> hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::allocate(Args&&... args) noexcept {
    if (this->allocatedSize_ == this->totalSize_) {
        std::size_t new_size = this->totalSize_ * 2;
        auto ptr = std::make_unique<char[]>( (sizeof(T) + sizeof(std::uint32_t)) * new_size );
        if (!ptr)
            return nullptr;

        std::uint32_t original_total_size = this->totalSize_;

        if constexpr (!std::is_trivially_copyable_v<T>) {
            for (std::uint32_t i = 0; i < original_total_size; ++i) {
                auto* src = reinterpret_cast<T*>(this->parseAt(i).second);
                auto* dst = reinterpret_cast<T*>(this->parseAt(i, ptr.get()).second);
                std::construct_at(dst, std::move(*src));
                std::destroy_at(src);
            }
        } else {
            std::memcpy(ptr.get(), this->pool_.get(), (sizeof(T) + sizeof(std::uint32_t)) * original_total_size);
        }

        std::ptrdiff_t delta = reinterpret_cast<std::uint8_t*>(ptr.get()) - reinterpret_cast<std::uint8_t*>(this->pool_.get());
        offset_ += delta;

        this->totalSize_ = new_size;
        this->pool_.swap(ptr);

        this->next_ = original_total_size;

        for (std::uint32_t i = original_total_size; i < new_size; ++i) {
            auto block = this->parseAt(i);
            block.first = i + 1;
        }
        auto last = this->parseAt(new_size - 1);
        last.first = new_size - 1;
    }
    
    auto ptr = this->parseNext();
    std::construct_at(ptr, std::forward<Args>(args)...);
    return Ptr<T, ReallocationPolicy::OffsetRealloc>{subtractOffset(ptr), *this};
}

template<typename T>
void hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::free(Ptr<T, hpool::ReallocationPolicy::OffsetRealloc>& ptr) noexcept {
        if (!ptr)
                return;

        auto ptr_ = addOffset(ptr.ptr_);

        std::ptrdiff_t shift = reinterpret_cast<std::ptrdiff_t>(ptr_) - 
                reinterpret_cast<std::ptrdiff_t>(this->pool_.get());
        shift /= (sizeof(T) + sizeof(std::uint32_t));

        if (shift < 0 || static_cast<std::uint32_t>(shift) >= this->totalSize_) {
                return;
        }

        auto block = this->parseAt(shift);
        block.first = this->next_;
        this->next_ = shift;

        --this->allocatedSize_;
}

template<typename T>
void HPool::HPool<T