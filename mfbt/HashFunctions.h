












































#ifndef mozilla_HashFunctions_h
#define mozilla_HashFunctions_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Char16.h"
#include "mozilla/Types.h"

#include <stdint.h>

#ifdef __cplusplus
namespace mozilla {




static const uint32_t GoldenRatioU32 = 0x9E3779B9U;

inline uint32_t
RotateBitsLeft32(uint32_t value, uint8_t bits)
{
  MOZ_ASSERT(bits < 32);
  return (value << bits) | (value >> (32 - bits));
}

namespace detail {

inline uint32_t
AddU32ToHash(uint32_t hash, uint32_t value)
{
  






































  return GoldenRatioU32 * (RotateBitsLeft32(hash, 5) ^ value);
}




template<size_t PtrSize>
inline uint32_t
AddUintptrToHash(uint32_t hash, uintptr_t value);

template<>
inline uint32_t
AddUintptrToHash<4>(uint32_t hash, uintptr_t value)
{
  return AddU32ToHash(hash, static_cast<uint32_t>(value));
}

template<>
inline uint32_t
AddUintptrToHash<8>(uint32_t hash, uintptr_t value)
{
  






  uint32_t v1 = static_cast<uint32_t>(value);
  uint32_t v2 = static_cast<uint32_t>(static_cast<uint64_t>(value) >> 32);
  return AddU32ToHash(AddU32ToHash(hash, v1), v2);
}

} 








template<typename A>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, A a)
{
  



  return detail::AddU32ToHash(hash, a);
}

template<typename A>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, A* a)
{
  




  static_assert(sizeof(a) == sizeof(uintptr_t),
                "Strange pointer!");

  return detail::AddUintptrToHash<sizeof(uintptr_t)>(hash, uintptr_t(a));
}

template<>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
AddToHash(uint32_t hash, uintptr_t a)
{
  return detail::AddUintptrToHash<sizeof(uintptr_t)>(hash, a);
}

template<typename A, typename B>
MOZ_WARN_UNUSED_RESULT
uint32_t
AddToHash(uint32_t hash, A a, B b)
{
  return AddToHash(AddToHash(hash, a), b);
}

template<typename A, typename B, typename C>
MOZ_WARN_UNUSED_RESULT
uint32_t
AddToHash(uint32_t hash, A a, B b, C c)
{
  return AddToHash(AddToHash(hash, a, b), c);
}

template<typename A, typename B, typename C, typename D>
MOZ_WARN_UNUSED_RESULT
uint32_t
AddToHash(uint32_t hash, A a, B b, C c, D d)
{
  return AddToHash(AddToHash(hash, a, b, c), d);
}

template<typename A, typename B, typename C, typename D, typename E>
MOZ_WARN_UNUSED_RESULT
uint32_t
AddToHash(uint32_t hash, A a, B b, C c, D d, E e)
{
  return AddToHash(AddToHash(hash, a, b, c, d), e);
}








template<typename A>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashGeneric(A a)
{
  return AddToHash(0, a);
}

template<typename A, typename B>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashGeneric(A a, B b)
{
  return AddToHash(0, a, b);
}

template<typename A, typename B, typename C>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashGeneric(A a, B b, C c)
{
  return AddToHash(0, a, b, c);
}

template<typename A, typename B, typename C, typename D>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashGeneric(A a, B b, C c, D d)
{
  return AddToHash(0, a, b, c, d);
}

template<typename A, typename B, typename C, typename D, typename E>
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashGeneric(A a, B b, C c, D d, E e)
{
  return AddToHash(0, a, b, c, d, e);
}

namespace detail {

template<typename T>
uint32_t
HashUntilZero(const T* str)
{
  uint32_t hash = 0;
  for (T c; (c = *str); str++)
    hash = AddToHash(hash, c);
  return hash;
}

template<typename T>
uint32_t
HashKnownLength(const T* str, size_t length)
{
  uint32_t hash = 0;
  for (size_t i = 0; i < length; i++)
    hash = AddToHash(hash, str[i]);
  return hash;
}

} 







MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const char* str)
{
  return detail::HashUntilZero(str);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const char* str, size_t length)
{
  return detail::HashKnownLength(str, length);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const uint16_t* str)
{
  return detail::HashUntilZero(str);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const uint16_t* str, size_t length)
{
  return detail::HashKnownLength(str, length);
}

#ifdef MOZ_CHAR16_IS_NOT_WCHAR
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const char16_t* str)
{
  return detail::HashUntilZero(str);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const char16_t* str, size_t length)
{
  return detail::HashKnownLength(str, length);
}
#endif





#ifdef WIN32
MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const wchar_t* str)
{
  return detail::HashUntilZero(str);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const wchar_t* str, size_t length)
{
  return detail::HashKnownLength(str, length);
}
#endif







MOZ_WARN_UNUSED_RESULT
extern MFBT_API uint32_t
HashBytes(const void* bytes, size_t length);

} 
#endif 

#endif 
