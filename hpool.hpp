#pragma once

#include "hpool.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace HPool {
  template<typename... T>
  class Ptr;

  template<typename... T>
  class HPool;

  namespace __hpool_impl {
    template<typename T1>
    class IPointer;

    template<typename T>
    struct is_variant;
    template<typename T>
    struct is_variant : std::false_type { };
    template<typename... T>
    struct is_variant<std::variant<T...>> : std::true_type { };

    template<typename T>
    struct extract_ptr_type;

    template<typename T>
    struct extract_ptr_type<Ptr<T>> {
      using type = T;
    };

    struct PtrAccess {
      template<typename Ptr>
      auto GetPool(Ptr& ptr) {
        return ptr.pl;
      }

      template<typename Ptr>
      auto GetPtr(Ptr& ptr) {
        return ptr.m_ptr;
      }

      template<typename Pool>
      auto AddOffset(Pool* pl, auto arg) {
        return pl->AddOffset(arg);
      }

      template<typename Pool>
      auto SubtractOffset(Pool* pl, auto arg) {
        return pl->SubtractOffset(arg);
      }
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

    template<typename T>
    class IHPool {
    protected:
      friend class PtrAccess;

      template<typename... Ts>
      friend class Ptr;

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

      template<typename PoolPointer, typename... Args>
      PoolPointer InternalAllocate(Args... args) noexcept;

      template<typename PoolPointer>
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

    template<typename T>
    class IPointer {
    protected:
      friend class IHPool<T>;
      IPointer() = default;
      IPointer(const std::nullptr_t) : m_ptr(nullptr) { }
      IPointer(HPoolElemWrapper<T>* ptr) : m_ptr(ptr) { }
      IPointer(const IPointer<T>& other) : m_ptr(other.m_ptr) { }
      IPointer& operator=(const IPointer& other) {
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
  class HPool<T1, T2, Ts...> : public __hpool_impl::IHPool<std::variant<T1, T2, Ts...>> {
    using Type = std::variant<T1, T2, Ts...>;
    using BasePointerType = Ptr<std::variant<T1, T2, Ts...>>;
    using Base = __hpool_impl::IHPool<Type>;
  public:
    explicit HPool(std::uint32_t size)
      : Base(size) { }

    template<typename T, typename... Args>
    Ptr<T, Type> allocate(Args... args);

    template<typename T>
    void free(Ptr<T, Type>&);
  };

  template<typename T>
  class HPool<T> : public __hpool_impl::IHPool<T> {
    using Type = T;
    using PointerType = Ptr<Type>;
    using Base = __hpool_impl::IHPool<Type>;
  public:
    explicit HPool(std::uint32_t size)
      : Base(size) { }

    template<typename... Args>
    PointerType allocate(Args... args) noexcept;
    void free(Ptr<T>&) noexcept;
  };

  template<typename T>
  class Ptr<T>
    : public __hpool_impl::IPointer<T>
    , public __hpool_impl::PtrAccess {

    using Base = __hpool_impl::IPointer<T>;
    using Access = __hpool_impl::PtrAccess;
  public:
    Ptr() : Base(nullptr), pl(nullptr) { };
    explicit Ptr(const std::nullptr_t) : Base(nullptr) { }
    Ptr(__hpool_impl::HPoolElemWrapper<T>* ptr, __hpool_impl::IHPool<T>* pl)
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

    T& operator*() { return Access::AddOffset(pl, Base::m_ptr)->value; }
    T* operator->() { return &Access::AddOffset(pl, Base::m_ptr)->value; }
    T* operator+() = delete;
    T* operator-() = delete;
    bool operator==(const std::nullptr_t) const { return Base::operator==(nullptr); }
    bool operator!=(const std::nullptr_t) const { return Base::operator!=(nullptr); }
    bool operator==(const Ptr& ptr) const { return Base::operator==(ptr); }
    bool operator!=(const Ptr& ptr) const { return Base::operator!=(ptr); }
    explicit operator bool() { return Base::operator bool(); }
  private:
    __hpool_impl::IHPool<T>* pl;
  };

  template<typename ActiveT, typename T1, typename T2, typename... Ts>
  class Ptr<ActiveT, std::variant<T1, T2, Ts...>>
    : public __hpool_impl::IPointer<std::variant<T1, T2, Ts...>>
    , public __hpool_impl::PtrAccess {

    using Type = std::variant<T1, T2, Ts...>;
    using Base = __hpool_impl::IPointer<Type>;
    using Access = __hpool_impl::PtrAccess;
  public:
    explicit Ptr(Ptr<Type>&&);

    Ptr() = default;
    explicit Ptr(const std::nullptr_t) : Base(nullptr) { }
    Ptr(__hpool_impl::HPoolElemWrapper<Type>* ptr, __hpool_impl::IHPool<Type>* pl)
      : Base(ptr), pl(pl) { }
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
    ActiveT* operator->();
    ActiveT* operator+() = delete;
    ActiveT* operator-() = delete;
    bool operator==(const std::nullptr_t) const { return Base::operator==(nullptr); }
    bool operator!=(const std::nullptr_t) const { return Base::operator!=(nullptr); }
    bool operator==(const Ptr& ptr) const { return Base::operator==(ptr); }
    bool operator!=(const Ptr& ptr) const { return Base::operator!=(ptr); }
    explicit operator bool() { return Base::operator bool(); }
  private:
    __hpool_impl::IHPool<Type>* pl;
  };

  template<typename ActiveT, typename T1, typename T2, typename... Ts>
  class PtrCast : public __hpool_impl::PtrAccess {
    template<typename... Tss>
    friend class Ptr;
  public:
    using Type = std::variant<T1, T2, Ts...>;

    static Ptr<ActiveT, std::variant<T1, T2, Ts...>>
      CreateMultiTypePtr(Ptr<std::variant<T1, T2, Ts...>>& ptr);

    static Ptr<std::variant<T1, T2, Ts...>>
      CreateSingleTypePtr(Ptr<ActiveT, std::variant<T1, T2, Ts...>>& ptr);
  };
}



template<typename ActiveT, typename T1, typename T2, typename... Ts>
HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>
  HPool::PtrCast<ActiveT, T1, T2, Ts...>::CreateMultiTypePtr(Ptr<std::variant<T1, T2, Ts...>>& ptr) {
  Ptr<ActiveT, std::variant<T1, T2, Ts...>> new_ptr;
  new_ptr.pl = ptr.pl;
  new_ptr.m_ptr = ptr.m_ptr;
  return new_ptr;
}

template<typename ActiveT, typename T1, typename T2, typename... Ts>
HPool::Ptr<std::variant<T1, T2, Ts...>>
  HPool::PtrCast<ActiveT, T1, T2, Ts...>::CreateSingleTypePtr(Ptr<ActiveT, std::variant<T1, T2, Ts...>>& ptr) {
  Ptr<ActiveT, std::variant<T1, T2, Ts...>> new_ptr;
  new_ptr.pl = ptr.pl;
  new_ptr.m_ptr = ptr.m_ptr;
  return new_ptr;
}

template<typename ActiveT, typename T1, typename T2, typename... Ts>
HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::Ptr(Ptr<std::variant<T1, T2, Ts...>>&& ptr) {
  Base::m_ptr = ptr.m_ptr;
}

template <typename ActiveT, typename T1, typename T2, typename... Ts>
ActiveT& HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::operator*() {
  if constexpr (std::is_default_constructible_v<ActiveT>) {
    if (!std::holds_alternative<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value)) {
      Access::AddOffset(pl, Base::m_ptr)->value = ActiveT();
    }
  } else {
    if (!std::holds_alternative<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value)) {
      assert(0 && "ActiveT must be default constructible to use operator-> and default initialize new object in place");
    }
  }

  return std::get<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value);
}

template <typename ActiveT, typename T1, typename T2, typename... Ts>
ActiveT* HPool::Ptr<ActiveT, std::variant<T1, T2, Ts...>>::operator->() {
  if constexpr (std::is_default_constructible_v<ActiveT>) {
    if (!std::holds_alternative<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value)) {
      Access::AddOffset(pl, Base::m_ptr)->value = ActiveT();
    }
  } else {
    if (!std::holds_alternative<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value)) {
      assert(0 && "ActiveT must be default constructible to use operator-> and default initialize new object in place");
    }
  }

  return &std::get<ActiveT>(Access::AddOffset(pl, Base::m_ptr)->value);
}

template<typename T>
template<typename... Args>
HPool::Ptr<T>
HPool::HPool<T>::allocate(Args... args) noexcept {
  return Base::template InternalAllocate<Ptr<T>>(std::forward<Args>(args)...);
}

template <typename T>
void HPool::HPool<T>::free(Ptr<T>& ptr) noexcept {
  Base::InternalFree(ptr);
}

template<typename T1, typename T2, typename... Ts>
template<typename T, typename... Args>
HPool::Ptr<T, std::variant<T1, T2, Ts...>>
HPool::HPool<T1, T2, Ts...>::allocate(Args... args) {
  auto ptr = Base::template InternalAllocate<Ptr<T, std::variant<T1, T2, Ts...>>>(std::forward<Args>(args)...);
  return ptr;
}

template <typename T1, typename T2, typename... Ts>
template <typename T>
void HPool::HPool<T1, T2, Ts...>::free(Ptr<T, Type>& ptr) {
  Base::InternalFree(ptr);
}


template <typename T>
template <typename PointerImpl, typename... Args>
PointerImpl HPool::__hpool_impl::IHPool<T>::InternalAllocate(Args... args) noexcept {
  if (m_allocated == m_totalsize) {
    std::size_t new_size = m_totalsize * 2;
    auto ptr = std::make_unique<HPoolElemWrapper<T>[]>(new_size);
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

template <typename T>
template <typename PoolPointer>
void HPool::__hpool_impl::IHPool<T>::InternalFree(PoolPointer& ptr) noexcept {
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
