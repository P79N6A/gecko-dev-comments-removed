#ifndef nsCppSharedAllocator_h__
#define nsCppSharedAllocator_h__

#include "nsMemory.h"     
#include NEW_H					


  
#ifdef _MSC_VER
  #pragma warning( disable: 4514 )
#endif

#include <limits.h>


template <class T>
class nsCppSharedAllocator
    



  {
    public:
      typedef T          value_type;
      typedef size_t     size_type;
      typedef ptrdiff_t  difference_type;

      typedef T*         pointer;
      typedef const T*   const_pointer;

      typedef T&         reference;
      typedef const T&   const_reference;



      nsCppSharedAllocator() { }

     ~nsCppSharedAllocator() { }


      pointer
      address( reference r ) const
        {
          return &r;
        }

      const_pointer
      address( const_reference r ) const
        {
          return &r;
        }

      pointer
      allocate( size_type n, const void* =0 )
        {
          return reinterpret_cast<pointer>(nsMemory::Alloc(static_cast<PRUint32>(n*sizeof(T))));
        }

      void
      deallocate( pointer p, size_type  )
        {
          nsMemory::Free(p);
        }

      void
      construct( pointer p, const T& val )
        {
          new (p) T(val);
        }
      
      void
      destroy( pointer p )
        {
          p->~T();
        }

      size_type
      max_size() const
        {
          return ULONG_MAX / sizeof(T);
        }

  };


template <class T>
bool
operator==( const nsCppSharedAllocator<T>&, const nsCppSharedAllocator<T>& )
  {
    return PR_TRUE;
  }

template <class T>
bool
operator!=( const nsCppSharedAllocator<T>&, const nsCppSharedAllocator<T>& )
  {
    return PR_FALSE;
  }

#endif 
