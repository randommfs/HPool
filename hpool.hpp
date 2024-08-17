#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <utility>


namespace hpool {

	template <typename T, std::size_t BlockSize = sizeof(T)> 
	class HPool {
	public:

		// public API
		explicit HPool(std::uint32_t size);

		T* allocate() noexcept;
		void free(T* pointer) noexcept;

		std::uint32_t size() const noexcept;
		std::uint32_t allocated() const noexcept;

	private:
		T* parseNext() noexcept;
		std::pair<std::uint32_t&, T*> parseAt(std::uint32_t hint) noexcept;


	private:
		std::unique_ptr<char[]> pool_;
		std::uint32_t totalSize_;
		std::uint32_t allocatedSize_;
		std::uint32_t next_;

	};

} // namespace hpool


template <typename T, std::size_t BlockSize> 
hpool::HPool<T, BlockSize>::HPool(std::uint32_t size)
	: pool_(std::make_unique<char[]>(size * (sizeof(std::uint32_t) + BlockSize)))
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


template <typename T, std::size_t BlockSize> 
T *hpool::HPool<T, BlockSize>::allocate() noexcept {
	if (allocatedSize_ == totalSize_) {
		return nullptr;
	}

	return parseNext();
}


template <typename T, std::size_t BlockSize>
void hpool::HPool<T, BlockSize>::free(T *ptr) noexcept {
	std::uint32_t shift = reinterpret_cast<std::ptrdiff_t>(ptr) - 
		reinterpret_cast<std::ptrdiff_t>(pool_.get());
	shift /= sizeof(std::uint32_t) + BlockSize;

	auto block = parseAt(shift);
	block.first = next_;
	next_ = shift;

	--allocatedSize_;
}


template <typename T, std::size_t BlockSize>
std::uint32_t hpool::HPool<T, BlockSize>::size() const noexcept {
	return totalSize_;
}


template <typename T, std::size_t BlockSize>
std::uint32_t hpool::HPool<T, BlockSize>::allocated() const noexcept {
	return allocatedSize_;
}


template <typename T, std::size_t BlockSize>
std::pair<std::uint32_t&, T*> hpool::HPool<T, BlockSize>::parseAt(std::uint32_t hint) noexcept {
	char* shift = pool_.get() + hint * (sizeof(std::uint32_t) + BlockSize);
	return std::make_pair(
		std::ref(reinterpret_cast<std::uint32_t&>(*shift)), 
		reinterpret_cast<T*>(shift + sizeof(std::uint32_t))
	);
}


template <typename T, std::size_t BlockSize>
T* hpool::HPool<T, BlockSize>::parseNext() noexcept {
	auto block = parseAt(next_);

	next_ = block.first;
	++allocatedSize_;
	return block.second;
}