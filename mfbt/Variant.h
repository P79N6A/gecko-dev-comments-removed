







#include <new>

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"

#ifndef mozilla_Variant_h
#define mozilla_Variant_h

namespace mozilla {

template<typename... Ts>
class Variant;

namespace detail {



template<typename T, typename... Ts>
struct MaxSizeOf
{
  static const size_t size = sizeof(T) > MaxSizeOf<Ts...>::size
    ? sizeof(T)
    : MaxSizeOf<Ts...>::size;
};

template<typename T>
struct MaxSizeOf<T>
{
  static const size_t size = sizeof(T);
};






template<typename Needle, typename... Haystack>
struct IsVariant;

template<typename Needle>
struct IsVariant<Needle>
{
  static const bool value = false;
};

template<typename Needle, typename... Haystack>
struct IsVariant<Needle, Needle, Haystack...>
{
  static const bool value = true;
};

template<typename Needle, typename T, typename... Haystack>
struct IsVariant<Needle, T, Haystack...> : public IsVariant<Needle, Haystack...> { };





template<size_t N, typename T, typename U, typename Next, bool isMatch>
struct TagHelper;


template<size_t N, typename T, typename U, typename Next>
struct TagHelper<N, T, U, Next, false>
{
  static size_t tag() { return Next::template tag<U>(); }
};


template<size_t N, typename T, typename U, typename Next>
struct TagHelper<N, T, U, Next, true>
{
  static size_t tag() { return N; }
};






template<size_t N, typename... Ts>
struct VariantImplementation;


template<size_t N, typename T>
struct VariantImplementation<N, T> {
  template<typename U>
  static size_t tag() {
    static_assert(mozilla::IsSame<T, U>::value,
                  "mozilla::Variant: tag: bad type!");
    return N;
  }

  template<typename Variant>
  static void copyConstruct(void* aLhs, const Variant& aRhs) {
    new (aLhs) T(aRhs.template as<T>());
  }

  template<typename Variant>
  static void moveConstruct(void* aLhs, Variant&& aRhs) {
    new (aLhs) T(aRhs.template extract<T>());
  }

  template<typename Variant>
  static void destroy(Variant& aV) {
    aV.template as<T>().~T();
  }
};


template<size_t N, typename T, typename... Ts>
struct VariantImplementation<N, T, Ts...>
{
  
  using Next = VariantImplementation<N + 1, Ts...>;

  template<typename U>
  static size_t tag() {
    return TagHelper<N, T, U, Next, IsSame<T, U>::value>::tag();
  }

  template<typename Variant>
  static void copyConstruct(void* aLhs, const Variant& aRhs) {
    if (aRhs.template is<T>()) {
      new (aLhs) T(aRhs.template as<T>());
    } else {
      Next::copyConstruct(aLhs, aRhs);
    }
  }

  template<typename Variant>
  static void moveConstruct(void* aLhs, Variant&& aRhs) {
    if (aRhs.template is<T>()) {
      new (aLhs) T(aRhs.template extract<T>());
    } else {
      Next::moveConstruct(aLhs, aRhs);
    }
  }

  template<typename Variant>
  static void destroy(Variant& aV) {
    if (aV.template is<T>()) {
      aV.template as<T>().~T();
    } else {
      Next::destroy(aV);
    }
  }
};

} 


















































































template<typename... Ts>
class Variant
{
  using Impl = detail::VariantImplementation<0, Ts...>;
  using RawData = AlignedStorage<detail::MaxSizeOf<Ts...>::size>;

  
  
  size_t tag;

  
  RawData raw;

  void* ptr() {
    return reinterpret_cast<void*>(&raw);
  }

public:
  
  template<typename RefT,
           
           
           
           
           typename T = typename RemoveReference<typename RemoveConst<RefT>::Type>::Type,
           typename = typename EnableIf<detail::IsVariant<T, Ts...>::value, void>::Type>
  explicit Variant(RefT&& aT)
    : tag(Impl::template tag<T>())
  {
    new (ptr()) T(Forward<T>(aT));
  }

  
  explicit Variant(const Variant& aRhs)
    : tag(aRhs.tag)
  {
    Impl::copyConstruct(ptr(), aRhs);
  }

  
  explicit Variant(Variant&& aRhs)
    : tag(aRhs.tag)
  {
    Impl::moveConstruct(ptr(), Move(aRhs));
  }

  
  Variant& operator=(const Variant& aRhs) {
    MOZ_ASSERT(&aRhs != this, "self-assign disallowed");
    this->~Variant();
    new (this) Variant(aRhs);
    return *this;
  }

  
  Variant& operator=(Variant&& aRhs) {
    MOZ_ASSERT(&aRhs != this, "self-assign disallowed");
    this->~Variant();
    new (this) Variant(Move(aRhs));
    return *this;
  }

  ~Variant()
  {
    Impl::destroy(*this);
  }

  
  template<typename T>
  bool is() const {
    static_assert(detail::IsVariant<T, Ts...>::value,
                  "provided a type not found in this Variant's type list");
    return Impl::template tag<T>() == tag;
  }

  

  
  template<typename T>
  T& as() {
    static_assert(detail::IsVariant<T, Ts...>::value,
                  "provided a type not found in this Variant's type list");
    MOZ_ASSERT(is<T>());
    return *reinterpret_cast<T*>(&raw);
  }

  
  template<typename T>
  const T& as() const {
    static_assert(detail::IsVariant<T, Ts...>::value,
                  "provided a type not found in this Variant's type list");
    MOZ_ASSERT(is<T>());
    return *reinterpret_cast<const T*>(&raw);
  }

  





  template<typename T>
  T extract() {
    static_assert(detail::IsVariant<T, Ts...>::value,
                  "provided a type not found in this Variant's type list");
    MOZ_ASSERT(is<T>());
    return T(Move(as<T>()));
  }
};

} 

#endif 
