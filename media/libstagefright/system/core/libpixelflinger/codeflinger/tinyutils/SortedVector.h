















#ifndef ANDROID_PIXELFLINGER_SORTED_VECTOR_H
#define ANDROID_PIXELFLINGER_SORTED_VECTOR_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

#include "Vector.h"
#include "VectorImpl.h"
#include "TypeHelpers.h"



namespace android {
namespace tinyutils {

template <class TYPE>
class SortedVector : private SortedVectorImpl
{
public:
            typedef TYPE    value_type;
    
    


    
                            SortedVector();
                            SortedVector(const SortedVector<TYPE>& rhs);
    virtual                 ~SortedVector();

    
    const SortedVector<TYPE>&   operator = (const SortedVector<TYPE>& rhs) const;    
    SortedVector<TYPE>&         operator = (const SortedVector<TYPE>& rhs);    

    



    inline  void            clear()             { VectorImpl::clear(); }

    



    
    inline  size_t          size() const                { return VectorImpl::size(); }
    
    inline  bool            isEmpty() const             { return VectorImpl::isEmpty(); }
    
    inline  size_t          capacity() const            { return VectorImpl::capacity(); }
    
    inline  ssize_t         setCapacity(size_t size)    { return VectorImpl::setCapacity(size); }

    


     
    
    inline  const TYPE*     array() const;

    
    
            TYPE*           editArray();

            
            ssize_t         indexOf(const TYPE& item) const;
            
            
            size_t          orderOf(const TYPE& item) const;
            
    
    



    
    inline  const TYPE&     operator [] (size_t index) const;
    
    inline  const TYPE&     itemAt(size_t index) const;
    
            const TYPE&     top() const;
    
            const TYPE&     mirrorItemAt(ssize_t index) const;

    



            
            ssize_t         add(const TYPE& item);
            
            
            TYPE&           editItemAt(size_t index) {
                return *( static_cast<TYPE *>(VectorImpl::editItemLocation(index)) );
            }

            
            ssize_t         merge(const Vector<TYPE>& vector);
            ssize_t         merge(const SortedVector<TYPE>& vector);
            
            
            ssize_t         remove(const TYPE&);

    
    inline  ssize_t         removeItemsAt(size_t index, size_t count = 1);
    
    inline  ssize_t         removeAt(size_t index)  { return removeItemsAt(index); }
            
protected:
    virtual void    do_construct(void* storage, size_t num) const;
    virtual void    do_destroy(void* storage, size_t num) const;
    virtual void    do_copy(void* dest, const void* from, size_t num) const;
    virtual void    do_splat(void* dest, const void* item, size_t num) const;
    virtual void    do_move_forward(void* dest, const void* from, size_t num) const;
    virtual void    do_move_backward(void* dest, const void* from, size_t num) const;
    virtual int     do_compare(const void* lhs, const void* rhs) const;
};






template<class TYPE> inline
SortedVector<TYPE>::SortedVector()
    : SortedVectorImpl(sizeof(TYPE),
                ((traits<TYPE>::has_trivial_ctor   ? HAS_TRIVIAL_CTOR   : 0)
                |(traits<TYPE>::has_trivial_dtor   ? HAS_TRIVIAL_DTOR   : 0)
                |(traits<TYPE>::has_trivial_copy   ? HAS_TRIVIAL_COPY   : 0)
                |(traits<TYPE>::has_trivial_assign ? HAS_TRIVIAL_ASSIGN : 0))
                )
{
}

template<class TYPE> inline
SortedVector<TYPE>::SortedVector(const SortedVector<TYPE>& rhs)
    : SortedVectorImpl(rhs) {
}

template<class TYPE> inline
SortedVector<TYPE>::~SortedVector() {
    finish_vector();
}

template<class TYPE> inline
SortedVector<TYPE>& SortedVector<TYPE>::operator = (const SortedVector<TYPE>& rhs) {
    SortedVectorImpl::operator = (rhs);
    return *this; 
}

template<class TYPE> inline
const SortedVector<TYPE>& SortedVector<TYPE>::operator = (const SortedVector<TYPE>& rhs) const {
    SortedVectorImpl::operator = (rhs);
    return *this; 
}

template<class TYPE> inline
const TYPE* SortedVector<TYPE>::array() const {
    return static_cast<const TYPE *>(arrayImpl());
}

template<class TYPE> inline
TYPE* SortedVector<TYPE>::editArray() {
    return static_cast<TYPE *>(editArrayImpl());
}


template<class TYPE> inline
const TYPE& SortedVector<TYPE>::operator[](size_t index) const {
    assert( index<size() );
    return *(array() + index);
}

template<class TYPE> inline
const TYPE& SortedVector<TYPE>::itemAt(size_t index) const {
    return operator[](index);
}

template<class TYPE> inline
const TYPE& SortedVector<TYPE>::mirrorItemAt(ssize_t index) const {
    assert( (index>0 ? index : -index)<size() );
    return *(array() + ((index<0) ? (size()-index) : index));
}

template<class TYPE> inline
const TYPE& SortedVector<TYPE>::top() const {
    return *(array() + size() - 1);
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::add(const TYPE& item) {
    return SortedVectorImpl::add(&item);
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::indexOf(const TYPE& item) const {
    return SortedVectorImpl::indexOf(&item);
}

template<class TYPE> inline
size_t SortedVector<TYPE>::orderOf(const TYPE& item) const {
    return SortedVectorImpl::orderOf(&item);
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::merge(const Vector<TYPE>& vector) {
    return SortedVectorImpl::merge(reinterpret_cast<const VectorImpl&>(vector));
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::merge(const SortedVector<TYPE>& vector) {
    return SortedVectorImpl::merge(reinterpret_cast<const SortedVectorImpl&>(vector));
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::remove(const TYPE& item) {
    return SortedVectorImpl::remove(&item);
}

template<class TYPE> inline
ssize_t SortedVector<TYPE>::removeItemsAt(size_t index, size_t count) {
    return VectorImpl::removeItemsAt(index, count);
}



template<class TYPE>
void SortedVector<TYPE>::do_construct(void* storage, size_t num) const {
    construct_type( reinterpret_cast<TYPE*>(storage), num );
}

template<class TYPE>
void SortedVector<TYPE>::do_destroy(void* storage, size_t num) const {
    destroy_type( reinterpret_cast<TYPE*>(storage), num );
}

template<class TYPE>
void SortedVector<TYPE>::do_copy(void* dest, const void* from, size_t num) const {
    copy_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

template<class TYPE>
void SortedVector<TYPE>::do_splat(void* dest, const void* item, size_t num) const {
    splat_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(item), num );
}

template<class TYPE>
void SortedVector<TYPE>::do_move_forward(void* dest, const void* from, size_t num) const {
    move_forward_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

template<class TYPE>
void SortedVector<TYPE>::do_move_backward(void* dest, const void* from, size_t num) const {
    move_backward_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

template<class TYPE>
int SortedVector<TYPE>::do_compare(const void* lhs, const void* rhs) const {
    return compare_type( *reinterpret_cast<const TYPE*>(lhs), *reinterpret_cast<const TYPE*>(rhs) );
}

} 
} 




#endif 
