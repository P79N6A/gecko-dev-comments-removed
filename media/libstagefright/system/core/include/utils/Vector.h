















#ifndef ANDROID_VECTOR_H
#define ANDROID_VECTOR_H

#include <new>
#include <stdint.h>
#include <sys/types.h>

#include <cutils/log.h>

#include <utils/VectorImpl.h>
#include <utils/TypeHelpers.h>



namespace stagefright {

template <typename TYPE>
class SortedVector;







template <class TYPE>
class Vector : private VectorImpl
{
public:
            typedef TYPE    value_type;
    
    


    
                            Vector();
                            Vector(const Vector<TYPE>& rhs);
    explicit                Vector(const SortedVector<TYPE>& rhs);
    virtual                 ~Vector();

    
            Vector<TYPE>&           operator = (const Vector<TYPE>& rhs);    

            Vector<TYPE>&           operator = (const SortedVector<TYPE>& rhs);

            



    inline  void            clear()             { VectorImpl::clear(); }

    



    
    inline  size_t          size() const                { return VectorImpl::size(); }
    
    inline  bool            isEmpty() const             { return VectorImpl::isEmpty(); }
    
    inline  size_t          capacity() const            { return VectorImpl::capacity(); }
    
    inline  ssize_t         setCapacity(size_t size)    { return VectorImpl::setCapacity(size); }

    



    inline  ssize_t         resize(size_t size)         { return VectorImpl::resize(size); }

    


     
    
    inline  const TYPE*     array() const;
    
            TYPE*           editArray();
    
    



    
    inline  const TYPE&     operator [] (size_t index) const;
    
    inline  const TYPE&     itemAt(size_t index) const;
    
            const TYPE&     top() const;

    



    
            TYPE&           editItemAt(size_t index);
    
            TYPE&           editTop();

            


            
    
            ssize_t         insertVectorAt(const Vector<TYPE>& vector, size_t index);

    
            ssize_t         appendVector(const Vector<TYPE>& vector);


    
            ssize_t         insertArrayAt(const TYPE* array, size_t index, size_t length);

    
            ssize_t         appendArray(const TYPE* array, size_t length);

            


             
    
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
     
     inline status_t        sort(compar_t cmp) {
         return VectorImpl::sort((VectorImpl::compar_t)cmp);
     }
     inline status_t        sort(compar_r_t cmp, void* state) {
         return VectorImpl::sort((VectorImpl::compar_r_t)cmp, state);
     }

     
     inline size_t getItemSize() const { return itemSize(); }


     



     typedef TYPE* iterator;
     typedef TYPE const* const_iterator;

     inline iterator begin() { return editArray(); }
     inline iterator end()   { return editArray() + size(); }
     inline const_iterator begin() const { return array(); }
     inline const_iterator end() const   { return array() + size(); }
     inline void reserve(size_t n) { setCapacity(n); }
     inline bool empty() const{ return isEmpty(); }
     inline void push_back(const TYPE& item)  { insertAt(item, size(), 1); }
     inline void push_front(const TYPE& item) { insertAt(item, 0, 1); }
     inline iterator erase(iterator pos) {
         ssize_t index = removeItemsAt(pos-array());
         return begin() + index;
     }

protected:
    virtual void    do_construct(void* storage, size_t num) const;
    virtual void    do_destroy(void* storage, size_t num) const;
    virtual void    do_copy(void* dest, const void* from, size_t num) const;
    virtual void    do_splat(void* dest, const void* item, size_t num) const;
    virtual void    do_move_forward(void* dest, const void* from, size_t num) const;
    virtual void    do_move_backward(void* dest, const void* from, size_t num) const;
};



template<typename T> struct trait_trivial_move<Vector<T> > { enum { value = true }; };





template<class TYPE> inline
Vector<TYPE>::Vector()
    : VectorImpl(sizeof(TYPE),
                ((traits<TYPE>::has_trivial_ctor   ? HAS_TRIVIAL_CTOR   : 0)
                |(traits<TYPE>::has_trivial_dtor   ? HAS_TRIVIAL_DTOR   : 0)
                |(traits<TYPE>::has_trivial_copy   ? HAS_TRIVIAL_COPY   : 0))
                )
{
}

template<class TYPE> inline
Vector<TYPE>::Vector(const Vector<TYPE>& rhs)
    : VectorImpl(rhs) {
}

template<class TYPE> inline
Vector<TYPE>::Vector(const SortedVector<TYPE>& rhs)
    : VectorImpl(static_cast<const VectorImpl&>(rhs)) {
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
Vector<TYPE>& Vector<TYPE>::operator = (const SortedVector<TYPE>& rhs) {
    VectorImpl::operator = (static_cast<const VectorImpl&>(rhs));
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
    LOG_FATAL_IF(index>=size(),
            "%s: index=%u out of range (%u)", __PRETTY_FUNCTION__,
            int(index), int(size()));
    return *(array() + index);
}

template<class TYPE> inline
const TYPE& Vector<TYPE>::itemAt(size_t index) const {
    return operator[](index);
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
ssize_t Vector<TYPE>::insertArrayAt(const TYPE* array, size_t index, size_t length) {
    return VectorImpl::insertArrayAt(array, index, length);
}

template<class TYPE> inline
ssize_t Vector<TYPE>::appendArray(const TYPE* array, size_t length) {
    return VectorImpl::appendArray(array, length);
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

}; 




#endif 
