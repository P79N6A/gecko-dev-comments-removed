






































#ifndef nsTPtrArray_h__
#define nsTPtrArray_h__

#include "nsTArray.h"






template<class E>
class nsTPtrArray : public nsTArray<E*> {
  public:
    typedef nsTPtrArray<E> self_type;
    typedef nsTArray<E*> base_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::elem_type elem_type;
    typedef typename base_type::index_type index_type;

    
    
    

    nsTPtrArray() {}

    
    explicit nsTPtrArray(size_type capacity) {
      SetCapacity(capacity);
    }
    
    
    
    nsTPtrArray(const self_type& other) {
      AppendElements(other);
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

template<class E, PRUint32 N>
class nsAutoTPtrArray : public nsTPtrArray<E> {
  public:
    typedef nsTPtrArray<E> base_type;
    typedef typename base_type::Header Header;
    typedef typename base_type::elem_type elem_type;

    nsAutoTPtrArray() {
      base_type::mHdr = reinterpret_cast<Header*>(&mAutoBuf);
      base_type::mHdr->mLength = 0;
      base_type::mHdr->mCapacity = N;
      base_type::mHdr->mIsAutoArray = 1;

      NS_ASSERTION(base_type::GetAutoArrayBuffer() ==
                   reinterpret_cast<Header*>(&mAutoBuf),
                   "GetAutoArrayBuffer needs to be fixed");
    }

  protected:
    char mAutoBuf[sizeof(Header) + N * sizeof(elem_type)];
};

#endif  
