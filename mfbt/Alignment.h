







#ifndef mozilla_Alignment_h
#define mozilla_Alignment_h

#include <stddef.h>
#include <stdint.h>

namespace mozilla {





template<typename T>
class AlignmentFinder
{
  struct Aligner
  {
    char mChar;
    T mT;
  };

public:
  static const size_t alignment = sizeof(Aligner) - sizeof(T);
};

#define MOZ_ALIGNOF(T) mozilla::AlignmentFinder<T>::alignment











#if defined(__GNUC__)
#  define MOZ_ALIGNED_DECL(_type, _align) \
     _type __attribute__((aligned(_align)))
#elif defined(_MSC_VER)
#  define MOZ_ALIGNED_DECL(_type, _align) \
     __declspec(align(_align)) _type
#else
#  warning "We don't know how to align variables on this compiler."
#  define MOZ_ALIGNED_DECL(_type, _align) _type
#endif







template<size_t Align>
struct AlignedElem;






template<>
struct AlignedElem<1>
{
  MOZ_ALIGNED_DECL(uint8_t elem, 1);
};

template<>
struct AlignedElem<2>
{
  MOZ_ALIGNED_DECL(uint8_t elem, 2);
};

template<>
struct AlignedElem<4>
{
  MOZ_ALIGNED_DECL(uint8_t elem, 4);
};

template<>
struct AlignedElem<8>
{
  MOZ_ALIGNED_DECL(uint8_t elem, 8);
};

template<>
struct AlignedElem<16>
{
  MOZ_ALIGNED_DECL(uint8_t elem, 16);
};











template<size_t Nbytes>
struct AlignedStorage
{
  union U
  {
    char mBytes[Nbytes];
    uint64_t mDummy;
  } u;

  const void* addr() const { return u.mBytes; }
  void* addr() { return u.mBytes; }
};

template<typename T>
struct AlignedStorage2
{
  union U
  {
    char mBytes[sizeof(T)];
    uint64_t mDummy;
  } u;

  const T* addr() const { return reinterpret_cast<const T*>(u.mBytes); }
  T* addr() { return static_cast<T*>(static_cast<void*>(u.mBytes)); }
};

} 

#endif 
