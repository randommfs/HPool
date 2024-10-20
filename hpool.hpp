#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>


namespace hpool {

	/* All policies */
	enum ReallocationPolicy {
		NoReallocations,
		OffsetRealloc
	};

	namespace _internal {
		/* Generic pool interface. Holds generic function implementations */
		template<typename T>
		class IHPool {
		protected:
			std::unique_ptr<char[]> pool_;
			std::uint32_t totalSize_;
			std::uint32_t allocatedSize_;
			std::uint32_t next_;

			std::pair<std::uint32_t&, T*> parseAt(std::uint32_t hint) noexcept;
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

			virtual T* allocate() noexcept = 0;
			virtual void free(T* pointer) noexcept = 0;

			/* Implemented in interface - doesn't depend on derived classes */
			std::uint32_t size() const noexcept;
			std::uint32_t allocated() const noexcept;

			virtual ~IHPool() { }
		};

		template<typename T, typename... Args>
		class IDeleter {
			IDeleter(const IDeleter<T>& other) = default;
			IDeleter(IDeleter<T>&& other) = default;

			void operator()(T* ptr) const noexcept;
		};
	}

	template<typename T, ReallocationPolicy Policy>
	class HPool;

	template<typename T>
	class HPool<T, ReallocationPolicy::NoReallocations> : public _internal::IHPool<T> {
	private:

	public:
		explicit HPool(std::uint32_t);

		virtual T* allocate() noexcept override;
		virtual void free(T*) noexcept override;
	};


	template<typename T>
	class HPool<T, ReallocationPolicy::OffsetRealloc> : public _internal::IHPool<T> {
	private:
		std::ptrdiff_t offset_;

		T* addOffset(T*);
		T* subtractOffset(T*);
	public:
		explicit HPool(std::uint32_t);

		virtual T* allocate() noexcept;
		virtual void free(T*) noexcept;
	};

	template<typename T, ReallocationPolicy Policy, typename Enable = void>
	class Deleter;

	template<typename T>
	class Deleter<T, ReallocationPolicy::NoReallocations> : private _internal::IDeleter<T> {
	private:
		HPool<T, ReallocationPolicy::NoReallocations>* pool_;
	public:
		Deleter(HPool<T, ReallocationPolicy::NoReallocations>& pool);

		void operator()(T* ptr) const noexcept;
	};

	template<typename T>
	class Deleter<T, ReallocationPolicy::OffsetRealloc> : private _internal::IDeleter<T> {
	private:
		HPool<T, ReallocationPolicy::OffsetRealloc>* pool_;
	public:
		Deleter(HPool<T, ReallocationPolicy::OffsetRealloc>& pool);

		void operator()(T* ptr) const noexcept;
	};

	template<typename T, typename... Args>
	std::shared_ptr<T> make_shared(HPool<T, ReallocationPolicy::NoReallocations>& pool, Args... args);

	template<typename T, typename... Args>
	std::unique_ptr<T, hpool::Deleter<T, ReallocationPolicy::NoReallocations>> make_unique(HPool<T, ReallocationPolicy::NoReallocations>& pool, Args... args);

} // namespace hpool

// Generic implementations
template <typename T> 
std::uint32_t hpool::_internal::IHPool<T>::size() const noexcept {
	return totalSize_;
}

template <typename T> 
std::uint32_t hpool::_internal::IHPool<T>::allocated() const noexcept {
	return allocatedSize_;
}

template <typename T> 
std::pair<std::uint32_t&, T*> hpool::_internal::IHPool<T>::parseAt(std::uint32_t hint) noexcept {
	char* shift = reinterpret_cast<char*>(this->pool_.get() + hint * (sizeof(T) + sizeof(std::uint32_t)));
	return std::make_pair(
		std::ref(reinterpret_cast<std::uint32_t&>(*shift)),
		reinterpret_cast<T*>(shift + sizeof(std::uint32_t))
	);
}

template <typename T> 
T* hpool::_internal::IHPool<T>::parseNext() noexcept {
	auto block = parseAt(this->next_);

	this->next_ = block.first;
	++this->allocatedSize_;
	return block.second;
}

// HPool NoReallocations implementation
template<typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::HPool(std::uint32_t size)
	: hpool::_internal::IHPool<T>(size) { }

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::allocate() noexcept {
	if (this->allocatedSize_ == this->totalSize_) {
		return nullptr;
	}

	return this->parseNext();
}

template <typename T> 
void hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::free(T *ptr) noexcept {
	if (!ptr)
		return;
	
	std::ptrdiff_t shift = reinterpret_cast<std::ptrdiff_t>(ptr) - 
		reinterpret_cast<std::ptrdiff_t>(this->pool_.get());
	if (shift < 0 || shift >= this->totalSize_ * sizeof(T)) {
		return;
	}
	shift /= sizeof(T);

	auto block = this->parseAt(shift);
	block.first = this->next_;
	this->next_ = shift;

	--this->allocatedSize_;
}

// Deleter NoReallocations implementation
template<typename T>
hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations>::Deleter(hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>& pool)
	: pool_(pool)
{}

template<typename T>
void hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations>::operator()(T* ptr) const noexcept {
	pool_->free(ptr);
}

// STD helpers implemented with NoReallocation policy
template<typename T, typename... Args>
std::shared_ptr<T> hpool::make_shared(HPool<T, hpool::ReallocationPolicy::NoReallocations>& pool, Args... args) {
	hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations> deleter (&pool);
	T* ptr = pool.allocate();
	if (!ptr) {
		throw std::runtime_error("allocation failed");
	}

	std::construct_at<T>(ptr, std::forward<Args...>(args...));
	return std::shared_ptr<T>(ptr, deleter);
}

template<typename T, typename... Args>
std::unique_ptr<T, hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations>> hpool::make_unique(HPool<T, hpool::ReallocationPolicy::NoReallocations>& pool, Args... args) {
	T* ptr = pool.allocate();
	if (!ptr) {
		throw std::runtime_error("allocation failed");
	}

	std::construct_at<T>(ptr, std::forward<Args...>(args...));
	return std::unique_ptr<T, hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations>>(ptr, hpool::Deleter<T, hpool::ReallocationPolicy::NoReallocations>(&pool));
}

// HPool OffsetRealloc implementation
template<typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::HPool(std::uint32_t size)
	: hpool::_internal::IHPool<T>(size)
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
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::allocate() noexcept {
	if (this->allocatedSize_ == this->totalSize_) {
		std::size_t new_size = this->totalSize_ * 2;
		auto ptr = std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t)) * new_size);
		if (!ptr)
			return nullptr;
		std::copy_n(this->pool_.get(), this->totalSize_, ptr.get());
		offset_ = reinterpret_cast<std::uint8_t*>(ptr.get()) - reinterpret_cast<std::uint8_t*>(this->pool_.get());
		this->pool_.release();
		this->totalSize_ = new_size;
		this->pool_.swap(ptr);


		for (std::uint32_t i = (this->totalSize_ / 2) - 1; i < this->totalSize_; ++i)
			this->parseAt(i).first = i + 1;
		this->parseAt(this->totalSize_ - 1).first = this->totalSize_ - 1;
	}

	return subtractOffset(this->parseNext());
}

template<typename T>
void hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::free(T* ptr) noexcept {
	if (!ptr)
		return;

	ptr = addOffset(ptr);
	
	std::ptrdiff_t shift = reinterpret_cast<std::ptrdiff_t>(ptr) - 
		reinterpret_cast<std::ptrdiff_t>(this->pool_.get());
	if (shift < 0 || shift >= this->totalSize_ * sizeof(T)) {
		return;
	}
	shift /= sizeof(T);

	auto block = this->parseAt(shift);
	block.first = this->next_;
	this->next_ = shift;

	--this->allocatedSize_;
}