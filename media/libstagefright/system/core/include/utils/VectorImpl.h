















#ifndef ANDROID_VECTOR_IMPL_H
#define ANDROID_VECTOR_IMPL_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>





namespace stagefright {










class VectorImpl
{
public:
    enum { 
        HAS_TRIVIAL_CTOR    = 0x00000001,
        HAS_TRIVIAL_DTOR    = 0x00000002,
        HAS_TRIVIAL_COPY    = 0x00000004,
    };

                            VectorImpl(size_t itemSize, uint32_t flags);
                            VectorImpl(const VectorImpl& rhs);
    virtual                 ~VectorImpl();

    
            void            finish_vector();

            VectorImpl&     operator = (const VectorImpl& rhs);    
            
    
    inline  const void*     arrayImpl() const       { return mStorage; }
            void*           editArrayImpl();
            
    
    inline  size_t          size() const        { return mCount; }
    inline  bool            isEmpty() const     { return mCount == 0; }
            size_t          capacity() const;
            ssize_t         setCapacity(size_t size);
            ssize_t         resize(size_t size);

            
            ssize_t         insertVectorAt(const VectorImpl& vector, size_t index);
            ssize_t         appendVector(const VectorImpl& vector);
            ssize_t         insertArrayAt(const void* array, size_t index, size_t length);
            ssize_t         appendArray(const void* array, size_t length);
            
            
            ssize_t         insertAt(size_t where, size_t numItems = 1);
            ssize_t         insertAt(const void* item, size_t where, size_t numItems = 1);
            void            pop();
            void            push();
            void            push(const void* item);
            ssize_t         add();
            ssize_t         add(const void* item);
            ssize_t         replaceAt(size_t index);
            ssize_t         replaceAt(const void* item, size_t index);

            
            ssize_t         removeItemsAt(size_t index, size_t count = 1);
            void            clear();

            const void*     itemLocation(size_t index) const;
            void*           editItemLocation(size_t index);

            typedef int (*compar_t)(const void* lhs, const void* rhs);
            typedef int (*compar_r_t)(const void* lhs, const void* rhs, void* state);
            status_t        sort(compar_t cmp);
            status_t        sort(compar_r_t cmp, void* state);

protected:
            size_t          itemSize() const;
            void            release_storage();

    virtual void            do_construct(void* storage, size_t num) const = 0;
    virtual void            do_destroy(void* storage, size_t num) const = 0;
    virtual void            do_copy(void* dest, const void* from, size_t num) const = 0;
    virtual void            do_splat(void* dest, const void* item, size_t num) const = 0;
    virtual void            do_move_forward(void* dest, const void* from, size_t num) const = 0;
    virtual void            do_move_backward(void* dest, const void* from, size_t num) const = 0;
    
private:
        void* _grow(size_t where, size_t amount);
        void  _shrink(size_t where, size_t amount);

        inline void _do_construct(void* storage, size_t num) const;
        inline void _do_destroy(void* storage, size_t num) const;
        inline void _do_copy(void* dest, const void* from, size_t num) const;
        inline void _do_splat(void* dest, const void* item, size_t num) const;
        inline void _do_move_forward(void* dest, const void* from, size_t num) const;
        inline void _do_move_backward(void* dest, const void* from, size_t num) const;

            
            
            void *      mStorage;   
            size_t      mCount;     

    const   uint32_t    mFlags;
    const   size_t      mItemSize;
};



class SortedVectorImpl : public VectorImpl
{
public:
                            SortedVectorImpl(size_t itemSize, uint32_t flags);
                            SortedVectorImpl(const VectorImpl& rhs);
    virtual                 ~SortedVectorImpl();
    
    SortedVectorImpl&     operator = (const SortedVectorImpl& rhs);    

    
            ssize_t         indexOf(const void* item) const;

    
            size_t          orderOf(const void* item) const;

    
            ssize_t         add(const void* item);

    
            ssize_t         merge(const VectorImpl& vector);
            ssize_t         merge(const SortedVectorImpl& vector);
             
    
            ssize_t         remove(const void* item);
        
protected:
    virtual int             do_compare(const void* lhs, const void* rhs) const = 0;

private:
            ssize_t         _indexOrderOf(const void* item, size_t* order = 0) const;

            
            
            ssize_t         add();
            void            pop();
            void            push();
            void            push(const void* item);
            ssize_t         insertVectorAt(const VectorImpl& vector, size_t index);
            ssize_t         appendVector(const VectorImpl& vector);
            ssize_t         insertArrayAt(const void* array, size_t index, size_t length);
            ssize_t         appendArray(const void* array, size_t length);
            ssize_t         insertAt(size_t where, size_t numItems = 1);
            ssize_t         insertAt(const void* item, size_t where, size_t numItems = 1);
            ssize_t         replaceAt(size_t index);
            ssize_t         replaceAt(const void* item, size_t index);
};

}; 




#endif
