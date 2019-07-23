





































#ifndef txOwningArray_h__
#define txOwningArray_h__

#include "nsTPtrArray.h"





template<class E>
class txOwningArray : public nsTPtrArray<E>
{
public:
    typedef nsTPtrArray<E> base_type;
    typedef typename base_type::elem_type elem_type;

    ~txOwningArray()
    {
        elem_type* iter = base_type::Elements();
        elem_type* end = iter + base_type::Length();
        for (; iter < end; ++iter) {
            delete *iter;
        }
    }
  
};

#endif 
