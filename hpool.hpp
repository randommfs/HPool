#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <iostream>


namespace hpool {

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
			IDeleter(const IDeleter<T>& other) { }
			IDeleter(IDeleter<T>&& other) { }

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

		Ptr<T, ReallocationPolicy::NoReallocations> allocate() noexcept;
		void free(Ptr<T, ReallocationPolicy::NoReallocations>&) noexcept;
	};

	template<typename T>
	class Ptr<T, ReallocationPolicy::NoReallocations> : public _internal::IPointer<T, ReallocationPolicy::NoReallocations> {
	private:
		friend class HPool<T, ReallocationPolicy::NoReallocations>;
		T* ptr_;
		Ptr<T, ReallocationPolicy::NoReallocations>(T* ptr) : ptr_(ptr) { }
	public:
		Ptr<T, ReallocationPolicy::NoReallocations>() = default;
		Ptr<T, ReallocationPolicy::NoReallocations>(const std::nullptr_t) : ptr_(nullptr) { }
		Ptr<T, ReallocationPolicy::NoReallocations>(const Ptr<T, ReallocationPolicy::NoReallocations>& other) {
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
	class HPool<T, ReallocationPolicy::OffsetRealloc> final : public _internal::IHPool<T, ReallocationPolicy::NoReallocations> {
	private:
		friend class Ptr<T, ReallocationPolicy::OffsetRealloc>;
		std::ptrdiff_t offset_;

		T* addOffset(T*);
		T* subtractOffset(T*);
	public:
		explicit HPool(std::uint32_t);

		Ptr<T, ReallocationPolicy::OffsetRealloc> allocate() noexcept;
		void free(Ptr<T, ReallocationPolicy::OffsetRealloc>&) noexcept;
	};

	template<typename T>
	class Ptr<T, ReallocationPolicy::OffsetRealloc> : public _internal::IPointer<T, ReallocationPolicy::OffsetRealloc> {
	private:
		friend class HPool<T, ReallocationPolicy::OffsetRealloc>;
		HPool<T, ReallocationPolicy::OffsetRealloc>* pool_;
		T* ptr_;
		Ptr<T, ReallocationPolicy::OffsetRealloc>(T* ptr, HPool<T, ReallocationPolicy::OffsetRealloc>& pool) : ptr_(ptr), pool_(&pool) { }
	public:
		Ptr<T, ReallocationPolicy::OffsetRealloc>() = default;
		Ptr<T, ReallocationPolicy::OffsetRealloc>(const std::nullptr_t) : pool_(nullptr), ptr_(nullptr) { }
		Ptr<T, ReallocationPolicy::OffsetRealloc>(HPool<T, ReallocationPolicy::OffsetRealloc>& pool) : pool_(&pool), ptr_(nullptr) { }
		Ptr<T, ReallocationPolicy::OffsetRealloc>(const Ptr<T, ReallocationPolicy::OffsetRealloc>& other) {
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
hpool::Ptr<T, hpool::ReallocationPolicy::NoReallocations> hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::allocate() noexcept {
	if (this->allocatedSize_ == this->totalSize_) {
		return nullptr;
	}

	return Ptr<T, NoReallocations>(this->parseNext());
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
	: hpool::_internal::IHPool<T, hpool::ReallocationPolicy::NoReallocations>(size)
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
hpool::Ptr<T, hpool::ReallocationPolicy::OffsetRealloc> hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::allocate() noexcept {
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

    return Ptr<T, ReallocationPolicy::OffsetRealloc>{subtractOffset(this->parseNext()), *this};
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
