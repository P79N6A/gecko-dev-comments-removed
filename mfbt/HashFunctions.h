













































#ifndef mozilla_HashFunctions_h
#define mozilla_HashFunctions_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Char16.h"
#include "mozilla/Types.h"

#include <stdint.h>

#ifdef __cplusplus
namespace mozilla {




static const uint32_t kGoldenRatioU32 = 0x9E3779B9U;

inline uint32_t
RotateBitsLeft32(uint32_t aValue, uint8_t aBits)
{
  MOZ_ASSERT(aBits < 32);
  return (aValue << aBits) | (aValue >> (32 - aBits));
}

namespace detail {

inline uint32_t
AddU32ToHash(uint32_t aHash, uint32_t aValue)
{
  






































  return kGoldenRatioU32 * (RotateBitsLeft32(aHash, 5) ^ aValue);
}




template<size_t PtrSize>
inline uint32_t
AddUintptrToHash(uint32_t aHash, uintptr_t aValue);

template<>
inline uint32_t
AddUintptrToHash<4>(uint32_t aHash, uintptr_t aValue)
{
  return AddU32ToHash(aHash, static_cast<uint32_t>(aValue));
}

template<>
inline uint32_t
AddUintptrToHash<8>(uint32_t aHash, uintptr_t aValue)
{
  






  uint32_t v1 = static_cast<uint32_t>(aValue);
  uint32_t v2 = static_cast<uint32_t>(static_cast<uint64_t>(aValue) >> 32);
  return AddU32ToHash(AddU32ToHash(aHash, v1), v2);
}

} 








template<typename A>
MOZ_WARN_UNUSED_RESULT inline uint32_t
AddToHash(uint32_t aHash, A aA)
{
  



  return detail::AddU32ToHash(aHash, aA);
}

template<typename A>
MOZ_WARN_UNUSED_RESULT inline uint32_t
AddToHash(uint32_t aHash, A* aA)
{
  




  static_assert(sizeof(aA) == sizeof(uintptr_t), "Strange pointer!");

  return detail::AddUintptrToHash<sizeof(uintptr_t)>(aHash, uintptr_t(aA));
}

template<>
MOZ_WARN_UNUSED_RESULT inline uint32_t
AddToHash(uint32_t aHash, uintptr_t aA)
{
  return detail::AddUintptrToHash<sizeof(uintptr_t)>(aHash, aA);
}

template<typename A, typename B>
MOZ_WARN_UNUSED_RESULT uint32_t
AddToHash(uint32_t aHash, A aA, B aB)
{
  return AddToHash(AddToHash(aHash, aA), aB);
}

template<typename A, typename B, typename C>
MOZ_WARN_UNUSED_RESULT uint32_t
AddToHash(uint32_t aHash, A aA, B aB, C aC)
{
  return AddToHash(AddToHash(aHash, aA, aB), aC);
}

template<typename A, typename B, typename C, typename D>
MOZ_WARN_UNUSED_RESULT uint32_t
AddToHash(uint32_t aHash, A aA, B aB, C aC, D aD)
{
  return AddToHash(AddToHash(aHash, aA, aB, aC), aD);
}

template<typename A, typename B, typename C, typename D, typename E>
MOZ_WARN_UNUSED_RESULT uint32_t
AddToHash(uint32_t aHash, A aA, B aB, C aC, D aD, E aE)
{
  return AddToHash(AddToHash(aHash, aA, aB, aC, aD), aE);
}








template<typename A>
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashGeneric(A aA)
{
  return AddToHash(0, aA);
}

template<typename A, typename B>
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashGeneric(A aA, B aB)
{
  return AddToHash(0, aA, aB);
}

template<typename A, typename B, typename C>
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashGeneric(A aA, B aB, C aC)
{
  return AddToHash(0, aA, aB, aC);
}

template<typename A, typename B, typename C, typename D>
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashGeneric(A aA, B aB, C aC, D aD)
{
  return AddToHash(0, aA, aB, aC, aD);
}

template<typename A, typename B, typename C, typename D, typename E>
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashGeneric(A aA, B aB, C aC, D aD, E aE)
{
  return AddToHash(0, aA, aB, aC, aD, aE);
}

namespace detail {

template<typename T>
uint32_t
HashUntilZero(const T* aStr)
{
  uint32_t hash = 0;
  for (T c; (c = *aStr); aStr++) {
    hash = AddToHash(hash, c);
  }
  return hash;
}

template<typename T>
uint32_t
HashKnownLength(const T* aStr, size_t aLength)
{
  uint32_t hash = 0;
  for (size_t i = 0; i < aLength; i++) {
    hash = AddToHash(hash, aStr[i]);
  }
  return hash;
}

} 







MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const char* aStr)
{
  return detail::HashUntilZero(reinterpret_cast<const unsigned char*>(aStr));
}

MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const char* aStr, size_t aLength)
{
  return detail::HashKnownLength(reinterpret_cast<const unsigned char*>(aStr), aLength);
}

MOZ_WARN_UNUSED_RESULT
inline uint32_t
HashString(const unsigned char* aStr, size_t aLength)
{
  return detail::HashKnownLength(aStr, aLength);
}

MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const uint16_t* aStr)
{
  return detail::HashUntilZero(aStr);
}

MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const uint16_t* aStr, size_t aLength)
{
  return detail::HashKnownLength(aStr, aLength);
}

#ifdef MOZ_CHAR16_IS_NOT_WCHAR
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const char16_t* aStr)
{
  return detail::HashUntilZero(aStr);
}

MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const char16_t* aStr, size_t aLength)
{
  return detail::HashKnownLength(aStr, aLength);
}
#endif





#ifdef WIN32
MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const wchar_t* aStr)
{
  return detail::HashUntilZero(aStr);
}

MOZ_WARN_UNUSED_RESULT inline uint32_t
HashString(const wchar_t* aStr, size_t aLength)
{
  return detail::HashKnownLength(aStr, aLength);
}
#endif







MOZ_WARN_UNUSED_RESULT extern MFBT_API uint32_t
HashBytes(const void* bytes, size_t aLength);

} 
#endif 

#endif 
