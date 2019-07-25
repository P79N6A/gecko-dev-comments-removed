






































#ifndef nsTPtrArray_h__
#define nsTPtrArray_h__

#include "nsTArray.h"






template<class E, class Alloc=nsTArrayDefaultAllocator>
class nsTPtrArray : public nsTArray<E*, Alloc> {
  public:
  typedef nsTPtrArray<E, Alloc> self_type;
  typedef nsTArray<E*, Alloc> base_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::elem_type elem_type;
    typedef typename base_type::index_type index_type;

    
    
    

    nsTPtrArray() {}

    
    explicit nsTPtrArray(size_type capacity) {
      this->SetCapacity(capacity);
    }
    
    
    
    nsTPtrArray(const self_type& other) {
      this->AppendElements(other);
    }

    
    
    

    
    elem_type& SafeElementAt(index_type i, elem_type& def) {
      return base_type::SafeElementAt(i, def);
    }
    const elem_type& SafeElementAt(index_type i, const elem_type& def) const {
      return base_type::SafeElementAt(i, def);
    }

    
    
    
    
    elem_type SafeElementAt(index_type i) const {
      return SafeElementAt(i, nsnull);
    }
};

template<class E, PRUint32 N, class Alloc=nsTArrayDefaultAllocator>
class nsAutoTPtrArray : public nsTPtrArray<E, Alloc> {
  public:
    typedef nsTPtrArray<E, Alloc> base_type;
    typedef typename base_type::Header Header;
    typedef typename base_type::elem_type elem_type;

    nsAutoTPtrArray() {
      *base_type::PtrToHdr() = reinterpret_cast<Header*>(&mAutoBuf);
      base_type::Hdr()->mLength = 0;
      base_type::Hdr()->mCapacity = N;
      base_type::Hdr()->mIsAutoArray = 1;

      NS_ASSERTION(base_type::GetAutoArrayBuffer() ==
                   reinterpret_cast<Header*>(&mAutoBuf),
                   "GetAutoArrayBuffer needs to be fixed");
    }

  protected:
    union {
      char mAutoBuf[sizeof(Header) + N * sizeof(elem_type)];
      PRUint64 dummy;
    };
};

#endif  
