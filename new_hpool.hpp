#pragma once

#include "new_hpool.hpp"

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
  struct proxy {};

  template<typename T>
  struct is_variant;
  template<typename T>
  struct is_variant : std::false_type { };
  template<typename... T>
  struct is_variant<std::variant<T...>> : std::true_type { };

  template<typename T>
  struct is_proxy;
  template<typename T>
  struct is_proxy : std::false_type { };
  template<typename... T>
  struct is_proxy<proxy<T...>> : std::true_type { };

  template<typename T>
  class HPool;

  template<typename T1, typename T2, typename... Ts>
  class HPool<proxy<T1, T2, Ts...>> : public __hpool_impl::IHPool<std::variant<T1, T2, Ts...>, Ptr<std::variant<T1, T2, Ts...>>> {
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

  template<typename T> requires (!is_proxy<T>::value)
  class HPool<T> : public __hpool_impl::IHPool<T, Ptr<T>> {
    using Type = T;
    using PointerType = Ptr<Type>;
    using Base = __hpool_impl::IHPool<Type, PointerType>;
  public:
    explicit HPool(std::uint32_t);

    template<typename... Args>
    PointerType allocate(Args... args) noexcept;
    void free(Ptr<T>&) noexcept;
  };

  template<typename T>
  class Ptr : public __hpool_impl::IPointer<T> {

  };
}

template<typename T>
template<typename... Args>
HPool::Ptr<T>
HPool::HPool<T>::allocate(Args... args) noexcept {

}

template<typename T1, typename T2, typename... Ts>
template<typename T, typename... Args>
HPool::Ptr<std::variant<T1, T2, Ts...>>
HPool::HPool<HPool::proxy<T1, T2, Ts...>>::allocate(Args... args) {
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

