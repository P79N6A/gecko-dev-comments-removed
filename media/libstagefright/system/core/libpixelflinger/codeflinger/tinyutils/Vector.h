















#ifndef ANDROID_PIXELFLINGER_VECTOR_H
#define ANDROID_PIXELFLINGER_VECTOR_H

#include <new>
#include <stdint.h>
#include <sys/types.h>

#include <cutils/log.h>

#include "Errors.h"
#include "VectorImpl.h"
#include "TypeHelpers.h"



namespace stagefright {
namespace tinyutils {







template <class TYPE>
class Vector : private VectorImpl
{
public:
            typedef TYPE    value_type;
    
    


    
                            Vector();
                            Vector(const Vector<TYPE>& rhs);
    virtual                 ~Vector();

    
            const Vector<TYPE>&     operator = (const Vector<TYPE>& rhs) const;
            Vector<TYPE>&           operator = (const Vector<TYPE>& rhs);    

    



    inline  void            clear()             { VectorImpl::clear(); }

    



    
    inline  size_t          size() const                { return VectorImpl::size(); }
    
    inline  bool            isEmpty() const             { return VectorImpl::isEmpty(); }
    
    inline  size_t          capacity() const            { return VectorImpl::capacity(); }
    
    inline  ssize_t         setCapacity(size_t size)    { return VectorImpl::setCapacity(size); }

    


     
    
    inline  const TYPE*     array() const;
    
            TYPE*           editArray();
    
    



    
    inline  const TYPE&     operator [] (size_t index) const;
    
    inline  const TYPE&     itemAt(size_t index) const;
    
            const TYPE&     top() const;
    
            const TYPE&     mirrorItemAt(ssize_t index) const;

    



    
            TYPE&           editItemAt(size_t index);
    
            TYPE&           editTop();

            


            
    
            ssize_t         insertVectorAt(const Vector<TYPE>& vector, size_t index);

    
            ssize_t         appendVector(const Vector<TYPE>& vector);


            


             
    
    inline  ssize_t         insertAt(size_t index, size_t numItems = 1);
    
            ssize_t         insertAt(const TYPE& prototype_item, size_t index, size_t numItems = 1);
    
    inline  void            pop();
    
    inline  void            push();
    
            void            push(const TYPE& item);
    
    inline  ssize_t         add();
    
            ssize_t         add(const TYPE& item);            
    
    inline  ssize_t         replaceAt(size_t index);
    
            ssize_t         replaceAt(const TYPE& item, size_t index);

    



    
    inline  ssize_t         removeItemsAt(size_t index, size_t count = 1);
    
    inline  ssize_t         removeAt(size_t index)  { return removeItemsAt(index); }

    


     
     typedef int (*compar_t)(const TYPE* lhs, const TYPE* rhs);
     typedef int (*compar_r_t)(const TYPE* lhs, const TYPE* rhs, void* state);
     
     inline status_t        sort(compar_t cmp);
     inline status_t        sort(compar_r_t cmp, void* state);

protected:
    virtual void    do_construct(void* storage, size_t num) const;
    virtual void    do_destroy(void* storage, size_t num) const;
    virtual void    do_copy(void* dest, const void* from, size_t num) const;
    virtual void    do_splat(void* dest, const void* item, size_t num) const;
    virtual void    do_move_forward(void* dest, const void* from, size_t num) const;
    virtual void    do_move_backward(void* dest, const void* from, size_t num) const;
};






template<class TYPE> inline
Vector<TYPE>::Vector()
    : VectorImpl(sizeof(TYPE),
                ((traits<TYPE>::has_trivial_ctor   ? HAS_TRIVIAL_CTOR   : 0)
                |(traits<TYPE>::has_trivial_dtor   ? HAS_TRIVIAL_DTOR   : 0)
                |(traits<TYPE>::has_trivial_copy   ? HAS_TRIVIAL_COPY   : 0)
                |(traits<TYPE>::has_trivial_assign ? HAS_TRIVIAL_ASSIGN : 0))
                )
{
}

template<class TYPE> inline
Vector<TYPE>::Vector(const Vector<TYPE>& rhs)
    : VectorImpl(rhs) {
}

template<class TYPE> inline
Vector<TYPE>::~Vector() {
    finish_vector();
}

template<class TYPE> inline
Vector<TYPE>& Vector<TYPE>::operator = (const Vector<TYPE>& rhs) {
    VectorImpl::operator = (rhs);
    return *this; 
}

template<class TYPE> inline
const Vector<TYPE>& Vector<TYPE>::operator = (const Vector<TYPE>& rhs) const {
    VectorImpl::operator = (rhs);
    return *this; 
}

template<class TYPE> inline
const TYPE* Vector<TYPE>::array() const {
    return static_cast<const TYPE *>(arrayImpl());
}

template<class TYPE> inline
TYPE* Vector<TYPE>::editArray() {
    return static_cast<TYPE *>(editArrayImpl());
}


template<class TYPE> inline
const TYPE& Vector<TYPE>::operator[](size_t index) const {
    LOG_FATAL_IF( index>=size(),
                  "itemAt: index %d is past size %d", (int)index, (int)size() );
    return *(array() + index);
}

template<class TYPE> inline
const TYPE& Vector<TYPE>::itemAt(size_t index) const {
    return operator[](index);
}

template<class TYPE> inline
const TYPE& Vector<TYPE>::mirrorItemAt(ssize_t index) const {
    LOG_FATAL_IF( (index>0 ? index : -index)>=size(),
                  "mirrorItemAt: index %d is past size %d",
                  (int)index, (int)size() );
    return *(array() + ((index<0) ? (size()-index) : index));
}

template<class TYPE> inline
const TYPE& Vector<TYPE>::top() const {
    return *(array() + size() - 1);
}

template<class TYPE> inline
TYPE& Vector<TYPE>::editItemAt(size_t index) {
    return *( static_cast<TYPE *>(editItemLocation(index)) );
}

template<class TYPE> inline
TYPE& Vector<TYPE>::editTop() {
    return *( static_cast<TYPE *>(editItemLocation(size()-1)) );
}

template<class TYPE> inline
ssize_t Vector<TYPE>::insertVectorAt(const Vector<TYPE>& vector, size_t index) {
    return VectorImpl::insertVectorAt(reinterpret_cast<const VectorImpl&>(vector), index);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::appendVector(const Vector<TYPE>& vector) {
    return VectorImpl::appendVector(reinterpret_cast<const VectorImpl&>(vector));
}

template<class TYPE> inline
ssize_t Vector<TYPE>::insertAt(const TYPE& item, size_t index, size_t numItems) {
    return VectorImpl::insertAt(&item, index, numItems);
}

template<class TYPE> inline
void Vector<TYPE>::push(const TYPE& item) {
    return VectorImpl::push(&item);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::add(const TYPE& item) {
    return VectorImpl::add(&item);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::replaceAt(const TYPE& item, size_t index) {
    return VectorImpl::replaceAt(&item, index);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::insertAt(size_t index, size_t numItems) {
    return VectorImpl::insertAt(index, numItems);
}

template<class TYPE> inline
void Vector<TYPE>::pop() {
    VectorImpl::pop();
}

template<class TYPE> inline
void Vector<TYPE>::push() {
    VectorImpl::push();
}

template<class TYPE> inline
ssize_t Vector<TYPE>::add() {
    return VectorImpl::add();
}

template<class TYPE> inline
ssize_t Vector<TYPE>::replaceAt(size_t index) {
    return VectorImpl::replaceAt(index);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::removeItemsAt(size_t index, size_t count) {
    return VectorImpl::removeItemsAt(index, count);
}



template<class TYPE>
void Vector<TYPE>::do_construct(void* storage, size_t num) const {
    construct_type( reinterpret_cast<TYPE*>(storage), num );
}

template<class TYPE>
void Vector<TYPE>::do_destroy(void* storage, size_t num) const {
    destroy_type( reinterpret_cast<TYPE*>(storage), num );
}

template<class TYPE>
void Vector<TYPE>::do_copy(void* dest, const void* from, size_t num) const {
    copy_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

template<class TYPE>
void Vector<TYPE>::do_splat(void* dest, const void* item, size_t num) const {
    splat_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(item), num );
}

template<class TYPE>
void Vector<TYPE>::do_move_forward(void* dest, const void* from, size_t num) const {
    move_forward_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

template<class TYPE>
void Vector<TYPE>::do_move_backward(void* dest, const void* from, size_t num) const {
    move_backward_type( reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num );
}

} 
} 




#endif 
