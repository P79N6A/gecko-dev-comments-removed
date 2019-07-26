




















#ifndef _STLP_ALLOC_C
#define _STLP_ALLOC_C

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

#if defined (__WATCOMC__)
#  pragma warning 13 9
#  pragma warning 367 9
#  pragma warning 368 9
#endif

_STLP_BEGIN_NAMESPACE

template <class _Alloc>
void * _STLP_CALL __debug_alloc<_Alloc>::allocate(size_t __n) {
  size_t __total_extra = __extra_before_chunk() + __extra_after_chunk();
  size_t __real_n = __n + __total_extra;
  if (__real_n < __n) {
    
    _STLP_THROW_BAD_ALLOC;
  }
  __alloc_header *__result = (__alloc_header *)__allocator_type::allocate(__real_n);
  memset((char*)__result, __shred_byte, __real_n * sizeof(value_type));
  __result->__magic = __magic;
  __result->__type_size = sizeof(value_type);
  __result->_M_size = (_STLP_UINT32_T)__n;
  return ((char*)__result) + (long)__extra_before;
}

template <class _Alloc>
void  _STLP_CALL
__debug_alloc<_Alloc>::deallocate(void *__p, size_t __n) {
  __alloc_header * __real_p = (__alloc_header*)((char *)__p -(long)__extra_before);
  
  _STLP_VERBOSE_ASSERT(__real_p->__magic != __deleted_magic, _StlMsg_DBA_DELETED_TWICE)
  _STLP_VERBOSE_ASSERT(__real_p->__magic == __magic, _StlMsg_DBA_NEVER_ALLOCATED)
  _STLP_VERBOSE_ASSERT(__real_p->__type_size == 1,_StlMsg_DBA_TYPE_MISMATCH)
  _STLP_VERBOSE_ASSERT(__real_p->_M_size == __n, _StlMsg_DBA_SIZE_MISMATCH)
  
  unsigned char* __tmp;
  for (__tmp = (unsigned char*)(__real_p + 1); __tmp < (unsigned char*)__p; ++__tmp) {
    _STLP_VERBOSE_ASSERT(*__tmp == __shred_byte, _StlMsg_DBA_UNDERRUN)
  }

  size_t __real_n = __n + __extra_before_chunk() + __extra_after_chunk();

  for (__tmp= ((unsigned char*)__p) + __n * sizeof(value_type);
       __tmp < ((unsigned char*)__real_p) + __real_n ; ++__tmp) {
    _STLP_VERBOSE_ASSERT(*__tmp == __shred_byte, _StlMsg_DBA_OVERRUN)
  }

  
  __real_p->__magic = __deleted_magic;
  memset((char*)__p, __shred_byte, __n * sizeof(value_type));
  __allocator_type::deallocate(__real_p, __real_n);
}

_STLP_END_NAMESPACE

#endif 




