#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <utility>


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

			std::pair<std::uint32_t&, T*> parseAt(std::uint32_t hint) noexcept {
				char* shift = reinterpret_cast<char*>(this->pool_.get() + hint * (sizeof(T) + sizeof(std::uint32_t)));
				return std::make_pair(
					std::ref(reinterpret_cast<std::uint32_t&>(*shift)),
					reinterpret_cast<T*>(shift + sizeof(std::uint32_t))
				);
			}
			T* parseNext() noexcept {
				auto block = parseAt(this->next_);
				this->next_ = block.first;
				++this->allocatedSize_;
				return block.second;
			}

		public:
			explicit IHPool(std::uint32_t size)
				: pool_(std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t))* size))
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
			std::uint32_t size() const noexcept {
				return totalSize_;
			}
			std::uint32_t allocated() const noexcept {
				return allocatedSize_;
			}

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
	}

	template<typename T, ReallocationPolicy Policy>
	class HPool;

	template<typename T>
	class HPool<T, ReallocationPolicy::NoReallocations> : public _internal::IHPool<T> {
	public:
		using IHPool<T>::IHPool;

		virtual T* allocate() noexcept override {
			if (this->allocatedSize_ == this->totalSize_) {
				return nullptr;
			}

			return this->parseNext();
		}
		virtual void free(T*) noexcept override {
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
	};


	template<typename T>
	class HPool<T, ReallocationPolicy::OffsetRealloc> : public _internal::IHPool<T> {
	private:
		std::ptrdiff_t offset_;

		T* addOffset(T* ptr) {
			return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) + offset_);
		}
		T* subtractOffset(T* ptr) {
			return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) - offset_);
		}
	public:
		template<typename T>
		explicit HPool(std::uint32_t size)
			: IHPool<T>(size)
			, offset_(0) { }

		virtual T* allocate() noexcept {
			if (this->allocatedSize_ == this->totalSize_) {
				std::size_t new_size = this->totalSize_ * 2;
				auto ptr = std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t)) * new_size);
				if (!ptr)
					return nullptr;
				std::memcpy(ptr.get(), this->pool_.get(), (sizeof(T) + sizeof(std::uint32_t)) * this->totalSize_);
				offset_ = reinterpret_cast<std::uint8_t*>(ptr.get()) - reinterpret_cast<std::uint8_t*>(this->pool_.get());
				this->totalSize_ = new_size;
				this->pool_.swap(ptr);
				++this->next_;

				for (std::uint32_t i = (new_size / 2) - 1; i < this->totalSize_; ++i) {
					auto block = this->parseAt(i);
					block.first = i + 1;
				}
				auto last = this->parseAt(this->totalSize_ - 1);
				last.first = this->totalSize_ - 1;

			}

			return subtractOffset(this->parseNext());
		}
		virtual void free(T* ptr) noexcept {
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
	};

	template<typename T, ReallocationPolicy Policy, typename Enable = void>
	class Deleter;

	template<typename T, ReallocationPolicy ReallocPolicy>
	class Deleter<T, ReallocPolicy> : private _internal::IDeleter<T> {
	private:
		HPool<T, ReallocPolicy>* pool_;
	public:
		Deleter(HPool<T, ReallocPolicy>& pool) noexcept : pool_(&pool) {}

		void operator()(T* ptr) const noexcept {
			pool_->free(ptr);
		}
	};

	template<typename T, ReallocationPolicy ReallocPolicy, typename... Args>
	std::shared_ptr<T> make_shared(HPool<T, ReallocPolicy>& pool, Args&&... args) {
		T* ptr = pool.allocate();
		if (!ptr) {
			throw std::runtime_error("allocation failed");
		}
		return std::shared_ptr<T>(std::construct_at<T>(ptr, std::forward<Args>(args)...), Deleter<T, ReallocPolicy>(&pool));
	}

	template<typename T, ReallocationPolicy ReallocPolicy, typename... Args>
	std::unique_ptr<T, Deleter<T, ReallocPolicy>> make_unique(HPool<T, ReallocPolicy>& pool, Args&&... args) {
		T* ptr = pool.allocate();
		if (!ptr) {
			throw std::runtime_error("allocation failed");
		}
		return std::unique_ptr<T, Deleter<T, ReallocPolicy>>(std::construct_at<T>(ptr, std::forward<Args>(args)...), Deleter<T, ReallocPolicy>(pool));

	}

} // namespace hpool
