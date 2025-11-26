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
  template<typename... T>
  class Ptr;

  template<typename... T>
  class HPool;

  namespace __hpool_impl {
    template<typename T1>
    class IPointer;

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
    struct extract_ptr_type;

    template<typename T>
    struct extract_ptr_type<Ptr<T>> {
      using type = T;
    };

    template<typename Ptr>
    concept PoolPointer = requires
    {
      typename extract_ptr_type<Ptr>::type;
    } && std::is_base_of_v<Ptr, IPointer<typename extract_ptr_type<Ptr>::type>>;

    template<typename T>
    struct HPoolElemWrapper {
      T value;
      std::uint32_t next;
    };

    template<typename T, typename PoolPointer>
    class IHPool {
    protected:
      friend class IPointer<T>;
      friend class Ptr<T>;
      std::unique_ptr<HPoolElemWrapper<T>[]> m_pool;
      std::uint32_t m_totalsize;
      std::uint32_t m_allocated;
      std::uint32_t m_next;
      std::ptrdiff_t m_offset;
      bool m_bad;

      HPoolElemWrapper<T>* ParseAt(std::uint32_t hint, HPoolElemWrapper<T>* pl = nullptr) noexcept {
        auto* pool = pl ? pl : m_pool.get();
        return pool + hint;
      }

      HPoolElemWrapper<T>* ParseNext() noexcept {
        auto block = ParseAt(m_next);
        m_next = block->next;
        ++m_allocated;
        return block;
      }

      HPoolElemWrapper<T>* AddOffset(HPoolElemWrapper<T>* ptr) {
        return reinterpret_cast<HPoolElemWrapper<T>*>(reinterpret_cast<std::uint8_t*>(ptr) + m_offset);
      }

      HPoolElemWrapper<T>* SubtractOffset(HPoolElemWrapper<T>* ptr) {
        return reinterpret_cast<HPoolElemWrapper<T>*>(reinterpret_cast<std::uint8_t*>(ptr) - m_offset);
      }

      template<typename... Args>
      PoolPointer InternalAllocate(Args... args) noexcept;

      void InternalFree(PoolPointer&) noexcept;

    public:
      IHPool(std::uint32_t size)
        : m_pool(std::make_unique<HPoolElemWrapper<T>[]>(size))
        , m_totalsize(size)
        , m_allocated(0)
        , m_next(0)
        , m_bad(false)
        , m_offset(0) {

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
    protected:
      friend class IHPool<T, Ptr<T>>;
      IPointer() = default;
      IPointer(const std::nullptr_t) : m_ptr(nullptr) { }
      IPointer(HPoolElemWrapper<T>* ptr) : m_ptr(ptr) { }
      IPointer(const IPointer<T>& other) : m_ptr(other.m_ptr) { }
      IPointer& operator=(IPointer& other) {
        m_ptr = other.m_ptr;
        return *this;
      }

      bool operator==(const std::nullptr_t) const { return m_ptr == nullptr; }
      bool operator!=(const std::nullptr_t) const { return m_ptr != nullptr; }
      bool operator==(const IPointer& ptr) const { return m_ptr == ptr.m_ptr; }
      bool operator!=(const IPointer& ptr) const { return m_ptr != ptr.m_ptr; }
      explicit operator bool() { return m_ptr != nullptr; }
      HPoolElemWrapper<T>* m_ptr;
    };
  }

  template<typename T1, typename T2, typename... Ts>
  class HPool<T1, T2, Ts...> : public __hpool_impl::IHPool<std::variant<T1, T2, Ts...>, Ptr<std::variant<T1, T2, Ts...>>> {
    using Type = std::variant<T1, T2, Ts...>;
    using BasePointerType = Ptr<std::variant<T1, T2, Ts...>>;
    using Base = __hpool_impl::IHPool<Type, BasePointerType>;
  public:
    explicit HPool(std::uint32_t size)
      : Base(size) { }

    template<typename T, typename... Args>
    Ptr<T, Type> allocate(Args... args);

    template<typename T>
    void free(Ptr<T, Type>&);
  };

  template<typename T>
  class HPool<T> : public __hpool_impl::IHPool<T, Ptr<T>> {
    using Type = T;
    using PointerType = Ptr<Type>;
    using Base = __hpool_impl::IHPool<Type, PointerType>;
  public:
    explicit HPool(std::uint32_t size)
      : Base(size) { }

    template<typename... Args>
    PointerType allocate(Args... args) noexcept;
    void free(Ptr<T>&) noexcept;
  };

  template<typename T>
  class Ptr<T> : public __hpool_impl::IPointer<T> {
    using Base = __hpool_impl::IPointer<T>;

  public:
    Ptr() : Base(nullptr), pl(nullptr) { };
    explicit Ptr(const std::nullptr_t) : Base(nullptr) { }
    Ptr(__hpool_impl::HPoolElemWrapper<T>* ptr, __hpool_impl::IHPool<T, Ptr>* pl)
      : Base(ptr), pl(pl) { }
    Ptr(const Ptr& other) : Base(other) { }

    Ptr& operator=(Ptr& other) {
      Base::operator=(other);
      pl = other.pl;
      return *this;
    }

    Ptr& operator=(Ptr&& other) noexcept {
      Base::operator=(other);
      pl = other.pl;
      return *this;
    }

    T& operator*() { return pl->AddOffset(Base::m_ptr)->value; }
    T& operator->() { return pl->AddOffset(Base::m_ptr)->value; }
    T* operator+() = delete;
    T* operator-() = delete;
    bool operator==(const std::nullptr_t) const { return Base::operator==(nullptr); }
    bool operator!=(const std::nullptr_t) const { return Base::operator!=(nullptr); }
    bool operator==(const Ptr& ptr) const { return Base::operator==(ptr); }
    bool operator!=(const Ptr& ptr) const { return Base::operator!=(ptr); }
    explicit operator bool() { return Base::operator bool(); }
  private:
    __hpool_impl::IHPool<T, Ptr>* pl;
  };

  template<typename ActiveT, typename T1, typename T2, typename... Ts>
  class Ptr<ActiveT, std::variant<T1, T2, Ts...>> : public __hpool_impl::IPointer<std::variant<T1, T2, Ts...>> {
    using Type = std::variant<T1, T2, Ts...>;
    using Base = __hpool_impl::IPointer<Type>;

    friend class Ptr<std::variant<T1, T2, Ts...>>;
  public:
    explicit Ptr(Ptr<Type>&&);

    Ptr() = default;
    explicit Ptr(const std::nullptr_t) : Base(nullptr) { }
    Ptr(__hpool_impl::HPoolElemWrapper<Type>* ptr, __hpool_impl::IHPool<Type, Ptr>* pl)
      : Base::m_ptr(ptr), pl(pl) { }
    Ptr(const Ptr& other) noexcept : Base(other), pl(other.pl) { }
    Ptr(Ptr&& other) noexcept : Base(other), pl(other.pl) { }

    Ptr& operator=(const Ptr& other) noexcept {
      Base::operator=(other);
      pl = other.pl;
      return *this;
    }

    Ptr& operator=(Ptr&& other) noexcept {
      Base::operator=(other);
      pl = other.pl;
      return *this;
    }

    ActiveT& operator*();
    ActiveT& operator->();
    ActiveT* operator+() = delete;
    ActiveT* operator-() = delete;
    bool operator==(const std::nullptr_t) const { return Base::operator==(nullptr); }
    bool operator!=(const std::nullptr_t) const { return Base::operator!=(nullptr); }
    bool operator==(const Ptr& ptr) const { return Base::operator==(ptr); }
    bool operator!=(const Ptr& ptr) const { return Base::operator!=(ptr); }
    explicit operator bool() { return Base::operator bool(); }
  private:
    HPool<Type>* pl;
  };

  template<typename ActiveT, typename T>
  class PtrCast {
    template<typename... T1>
    friend class Ptr;
  public:
    static Ptr<ActiveT, T>
      CreateMultiTypePtr(Ptr<T>& ptr) {
      Ptr<ActiveT, T> new_ptr;
      new_ptr.pl = ptr.pl;
      new_ptr.m_ptr = ptr.m_ptr;
      return new_ptr;
    }
  };
}

template<typename ActiveT, typename T1, typename T2, typename... Ts>
HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::Ptr(Ptr<std::variant<T1, T2, Ts...>>&& ptr) {
  Base::m_ptr = ptr.m_ptr;
}

template <typename ActiveT, typename T1, typename T2, typename... Ts>
ActiveT& HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::operator*() {
  return std::get<ActiveT>(*Base::m_ptr);
}

template <typename ActiveT, typename T1, typename T2, typename... Ts>
ActiveT& HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::operator->() {
  return std::get<ActiveT>(*Base::m_ptr);
}

template<typename T>
template<typename... Args>
HPool::Ptr<T>
HPool::HPool<T>::allocate(Args... args) noexcept {
  return Base::InternalAllocate(std::forward<Args>(args)...);
}

template <typename T>
void HPool::HPool<T>::free(Ptr<T>& ptr) noexcept {
  Base::InternalFree(ptr);
}

template<typename T1, typename T2, typename... Ts>
template<typename T, typename... Args>
HPool::Ptr<T, std::variant<T1, T2, Ts...>>
HPool::HPool<T1, T2, Ts...>::allocate(Args... args) {
  auto ptr = Base::InternalAllocate(std::forward<Args>(args)...);
  return PtrCast<T, std::variant<T1, T2, Ts...>>::CreateMultiTypePtr(ptr);
  //return Ptr<T, std::variant<T1, T2, Ts...>>(ptr);
}

template <typename T1, typename T2, typename... Ts>
template <typename T>
void HPool::HPool<T1, T2, Ts...>::free(Ptr<T, Type>& ptr) {
  Base::InternalFree(ptr);
}


template <typename T, typename PointerImpl>
template <typename... Args>
PointerImpl HPool::__hpool_impl::IHPool<T, PointerImpl>::InternalAllocate(Args... args) noexcept {
  if (m_allocated == m_totalsize) {
    std::size_t new_size = m_totalsize * 2;
    auto ptr = std::make_unique<HPoolElemWrapper<T>[]>(sizeof(HPoolElemWrapper<T>) * new_size);
    if (!ptr) {
      return PointerImpl{nullptr};
    }

    std::uint32_t orig_size = m_totalsize;

    for (std::uint32_t i = 0; i < orig_size; ++i) {
      auto* src = &reinterpret_cast<HPoolElemWrapper<T>*>(ParseAt(i))->value;
      auto* dst = &reinterpret_cast<HPoolElemWrapper<T>*>(ParseAt(i, ptr.get()))->value;
      std::construct_at(dst, std::move(*src));
      std::destroy_at(src);
    }

    std::ptrdiff_t delta = reinterpret_cast<std::uint8_t*>(ptr.get()) - reinterpret_cast<std::uint8_t*>(m_pool.get());
    m_offset += delta;
    m_totalsize = new_size;
    m_pool.swap(ptr);
    m_next = orig_size;

    for (std::uint32_t i = orig_size; i < new_size; ++i) {
      auto* block = ParseAt(i);
      block->next = i + 1;
    }
    auto* last = ParseAt(new_size - 1);
    last->next = new_size - 1;
  }

  auto ptr = ParseNext();
  if constexpr (is_variant<T>::value) {
    std::construct_at(ptr, T{std::forward<Args>(args)...});
  } else {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return PointerImpl(SubtractOffset(ptr), this);
}

template <typename T, typename PoolPointer>
void HPool::__hpool_impl::IHPool<T, PoolPointer>::InternalFree(PoolPointer& ptr) noexcept {
  if (!ptr) {
    return;
  }

  auto offset_ptr = SubtractOffset(ptr.m_ptr);
  std::ptrdiff_t shift = reinterpret_cast<std::ptrdiff_t>(offset_ptr) -
    reinterpret_cast<std::ptrdiff_t>(m_pool.get());

  if (shift < 0 || static_cast<std::uint32_t>(shift) >= m_totalsize) {
    return;
  }

  auto block = ParseAt(shift);
  block->next = m_next;
  m_next = shift;

  --m_allocated;
}
