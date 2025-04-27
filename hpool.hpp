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

namespace hpool {

  /* All policies */
  enum class ReallocationPolicy {
    NoReallocations,
    OffsetRealloc
  };

  namespace _internal {
    /* Generic pool: holds generic function implementations */
    template <typename T, ReallocationPolicy Policy>
    class HBasePool {
    public:
      /* Deleter interface: defines pool elements' deletion classes */
      template <typename... Args>
      class IDeleter {
      public:
        virtual void operator()(T* ptr) const noexcept = 0;
      };

      /* Pointer interface: defines access to pool element via proxy class */
      class IPointer {
      public:
        virtual T& operator*() = 0;
        virtual T& operator->() = 0;
        virtual T* operator+() = 0;
        virtual T* operator-() = 0;
        virtual operator bool() = 0;
      };

      HBasePool(std::uint32_t size);

      /* Implemented in base class - doesn't depend on derived classes */
      std::uint32_t size() const noexcept;
      std::uint32_t allocated() const noexcept;

      virtual ~HBasePool() = default;

    protected:
      std::pair<std::uint32_t&, T*> parseAt(std::uint32_t hint, char* ptr = nullptr) noexcept;
      T* parseNext() noexcept;

    protected:
      std::unique_ptr<char[]> pool_;
      std::uint32_t totalSize_;
      std::uint32_t allocatedSize_;
      std::uint32_t next_;
    };
  } // namespace _internal

  template <typename T, ReallocationPolicy Policy>
  class HPool;

  template <typename T>
  using HPoolNoRealloc = HPool<T, ReallocationPolicy::NoReallocations>;

  template <typename T>
  using HPoolOffsetRealloc = HPool<T, ReallocationPolicy::OffsetRealloc>;

  template <typename T>
  class HPool<T, ReallocationPolicy::NoReallocations>
      : public _internal::HBasePool<T, ReallocationPolicy::NoReallocations> {
  public:
    class Ptr : public _internal::HBasePool<T, ReallocationPolicy::NoReallocations>::IPointer {
    public:
      friend class HPool<T, ReallocationPolicy::NoReallocations>;

      Ptr() = default;
      Ptr(const std::nullptr_t);
      Ptr(const Ptr& other);
      Ptr& operator=(Ptr other);

      virtual T& operator*() override;
      virtual T& operator->() override;
      virtual T* operator+() override;
      virtual T* operator-() override;
      virtual explicit operator bool() override;

      bool operator==(const std::nullptr_t) const;
      bool operator!=(const std::nullptr_t) const;
      bool operator==(Ptr ptr) const;
      bool operator!=(Ptr ptr) const;

    private:
      Ptr(T* ptr);

    private:
      T* ptr_;
    };

    explicit HPool(std::uint32_t);

    template <typename... Args>
    Ptr allocate(Args&&... args) noexcept;
    void free(Ptr&) noexcept;
  };

  template <typename T>
  class HPool<T, ReallocationPolicy::OffsetRealloc> : public _internal::HBasePool<T, ReallocationPolicy::OffsetRealloc> {
  public:
    class Ptr : public _internal::HBasePool<T, ReallocationPolicy::OffsetRealloc>::IPointer {
    public:
      friend class HPool<T, ReallocationPolicy::OffsetRealloc>;

      Ptr() = default;
      Ptr(const std::nullptr_t);
      Ptr(HPool<T, ReallocationPolicy::OffsetRealloc>& pool);
      Ptr(const Ptr& other);
      Ptr& operator=(Ptr other);

      virtual T& operator*() override;
      virtual T& operator->() override;
      virtual T* operator+() override;
      virtual T* operator-() override;
      virtual explicit operator bool() override;

      bool operator==(const std::nullptr_t) const;
      bool operator!=(const std::nullptr_t) const;
      bool operator==(Ptr ptr) const;
      bool operator!=(Ptr ptr) const;

    private:
      Ptr(T* ptr, HPool<T, ReallocationPolicy::OffsetRealloc>& pool);

    private:
      HPool<T, ReallocationPolicy::OffsetRealloc>* pool_;
      T* ptr_;
    };

    friend class Ptr;

    explicit HPool(std::uint32_t);

    template <typename... Args>
    Ptr allocate(Args&&... args) noexcept;
    void free(Ptr&) noexcept;

  private:
    T* addOffset(T*);
    T* subtractOffset(T*);

  private:
    std::ptrdiff_t offset_;
  };

  template <typename T, ReallocationPolicy Policy, typename Enable = void>
  class Deleter;

  template <typename T, ReallocationPolicy ReallocPolicy>
  class Deleter<T, ReallocPolicy> : private _internal::HBasePool<T, ReallocPolicy>::IDeleter {
  public:
    Deleter(HPool<T, ReallocPolicy>& pool);
    virtual void operator()(T* ptr) const noexcept override;

  private:
    HPool<T, ReallocPolicy>* pool_;
  };

} // namespace hpool

template <typename T, hpool::ReallocationPolicy Policy>
hpool::_internal::HBasePool<T, Policy>::HBasePool(std::uint32_t size)
    : pool_(std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t)) * size)),
      totalSize_(size),
      allocatedSize_(0),
      next_(0) {
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

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::Ptr(T* ptr)
    : ptr_(ptr) {
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::Ptr(const std::nullptr_t)
    : ptr_(nullptr) {
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::Ptr(const Ptr& other) {
  ptr_ = other.ptr_;
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr& hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator=(Ptr other) {
  ptr_ = other.ptr_;
  return *this;
}

template <typename T>
T& hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator*() {
  return *ptr_;
}

template <typename T>
T& hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator->() {
  return *ptr_;
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator+() {
  return ptr_;
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator-() {
  return ptr_;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator==(const std::nullptr_t) const {
  return ptr_ == nullptr;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator!=(const std::nullptr_t) const {
  return ptr_ != nullptr;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator==(Ptr ptr) const {
  return ptr_ == ptr.ptr_;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator!=(Ptr ptr) const {
  return ptr_ != ptr.ptr_;
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr::operator bool() {
  return ptr_ != nullptr;
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::Ptr(T* ptr, HPool<T, ReallocationPolicy::OffsetRealloc>& pool)
    : pool_(&pool), ptr_(ptr) {
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::Ptr(const std::nullptr_t)
    : pool_(nullptr), ptr_(nullptr) {
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::Ptr(HPool<T, ReallocationPolicy::OffsetRealloc>& pool)
    : pool_(&pool), ptr_(nullptr) {
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::Ptr(const Ptr& other) {
  pool_ = other.pool_;
  ptr_ = other.ptr_;
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr& hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator=(Ptr other) {
  pool_ = other.pool_;
  ptr_ = other.ptr_;
  return *this;
}

template <typename T>
T& hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator*() {
  return *pool_->addOffset(ptr_);
}

template <typename T>
T& hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator->() {
  return *pool_->addOffset(ptr_);
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator+() {
  return pool_->addOffset(ptr_);
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator-() {
  return pool_->addOffset(ptr_);
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator==(const std::nullptr_t) const {
  return ptr_ == nullptr;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator!=(const std::nullptr_t) const {
  return ptr_ != nullptr;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator==(Ptr ptr) const {
  return ptr_ == ptr.ptr_;
}

template <typename T>
bool hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator!=(Ptr ptr) const {
  return ptr_ != ptr.ptr_;
}

template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr::operator bool() {
  return ptr_ != nullptr;
}

// Generic implementations
template <typename T, hpool::ReallocationPolicy Policy>
std::uint32_t hpool::_internal::HBasePool<T, Policy>::size() const noexcept {
  return totalSize_;
}

template <typename T, hpool::ReallocationPolicy Policy>
std::uint32_t hpool::_internal::HBasePool<T, Policy>::allocated() const noexcept {
  return allocatedSize_;
}

template <typename T, hpool::ReallocationPolicy Policy>
std::pair<std::uint32_t&, T*> hpool::_internal::HBasePool<T, Policy>::parseAt(std::uint32_t hint, char* pl) noexcept {
  char* pool = pl ? pl : this->pool_.get();
  char* shift = reinterpret_cast<char*>(pool + hint * (sizeof(T) + sizeof(std::uint32_t)));
  return std::make_pair(
      std::ref(reinterpret_cast<std::uint32_t&>(*shift)),
      reinterpret_cast<T*>(shift + sizeof(std::uint32_t)));
}

template <typename T, hpool::ReallocationPolicy Policy>
T* hpool::_internal::HBasePool<T, Policy>::parseNext() noexcept {
  auto block = parseAt(this->next_);

  this->next_ = block.first;
  ++this->allocatedSize_;
  return block.second;
}

// HPool NoReallocations implementation
template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::HPool(std::uint32_t size)
    : hpool::_internal::HBasePool<T, hpool::ReallocationPolicy::NoReallocations>(size) {
}

template <typename T>
template <typename... Args>
hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::allocate(Args&&... args) noexcept {
  if (this->allocatedSize_ == this->totalSize_) {
    return nullptr;
  }

  auto ptr = this->parseNext();
  std::construct_at(ptr, std::forward<Args>(args)...);
  return Ptr(ptr);
}

template <typename T>
void hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::free(hpool::HPool<T, hpool::ReallocationPolicy::NoReallocations>::Ptr& ptr) noexcept {
  if (!ptr) {
    return;
  }

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
template <typename T, hpool::ReallocationPolicy ReallocPolicy>
hpool::Deleter<T, ReallocPolicy>::Deleter(hpool::HPool<T, ReallocPolicy>& pool)
    : pool_(&pool) {
}

template <typename T, hpool::ReallocationPolicy ReallocPolicy>
void hpool::Deleter<T, ReallocPolicy>::operator()(T* ptr) const noexcept {
  pool_->free(ptr);
}

// HPool OffsetRealloc implementation
template <typename T>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::HPool(std::uint32_t size)
    : hpool::_internal::HBasePool<T, hpool::ReallocationPolicy::OffsetRealloc>(size), offset_(0) {
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::addOffset(T* ptr) {
  return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) + offset_);
}

template <typename T>
T* hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::subtractOffset(T* ptr) {
  return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(ptr) - offset_);
}

template <typename T>
template <typename... Args>
hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::allocate(Args&&... args) noexcept {
  if (this->allocatedSize_ == this->totalSize_) {
    std::size_t new_size = this->totalSize_ * 2;
    auto ptr = std::make_unique<char[]>((sizeof(T) + sizeof(std::uint32_t)) * new_size);
    if (!ptr) {
      return nullptr;
    }

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
  return Ptr(subtractOffset(ptr), *this);
}

template <typename T>
void hpool::HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::free(HPool<T, hpool::ReallocationPolicy::OffsetRealloc>::Ptr& ptr) noexcept {
  if (!ptr) {
    return;
  }

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
