





#ifndef AlignedTArray_h__
#define AlignedTArray_h__

#include "mozilla/Alignment.h"
#include "nsTArray.h"





template <typename E, int N = 32>
class AlignedTArray : public nsTArray_Impl<E, nsTArrayInfallibleAllocator>
{
  static_assert((N & (N-1)) == 0, "N must be power of 2");
  typedef nsTArray_Impl<E, nsTArrayInfallibleAllocator> base_type;
public:
  typedef E                                          elem_type;
  typedef typename base_type::size_type              size_type;
  typedef typename base_type::index_type             index_type;

  AlignedTArray() {}
  explicit AlignedTArray(size_type capacity) : base_type(capacity + sExtra) {}
  elem_type* Elements() { return getAligned(base_type::Elements()); }
  const elem_type* Elements() const { return getAligned(base_type::Elements()); }
  elem_type& operator[](index_type i) { return Elements()[i];}
  const elem_type& operator[](index_type i) const { return Elements()[i]; }

  void SetLength(size_type newLen)
  {
    base_type::SetLength(newLen + sExtra);
  }

  MOZ_WARN_UNUSED_RESULT
  bool SetLength(size_type newLen, const mozilla::fallible_t&)
  {
    return base_type::SetLength(newLen + sExtra, mozilla::fallible);
  }

  size_type Length() const {
    return base_type::Length() <= sExtra ? 0 : base_type::Length() - sExtra;
  }

private:
  AlignedTArray(const AlignedTArray& other) = delete;
  void operator=(const AlignedTArray& other) = delete;

  static const size_type sPadding = N <= MOZ_ALIGNOF(E) ? 0 : N - MOZ_ALIGNOF(E);
  static const size_type sExtra = (sPadding + sizeof(E) - 1) / sizeof(E);

  template <typename U>
  static U* getAligned(U* p)
  {
    return reinterpret_cast<U*>(((uintptr_t)p + N - 1) & ~(N-1));
  }
};

#endif 
