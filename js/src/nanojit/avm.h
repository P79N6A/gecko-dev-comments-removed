































#ifndef avm_h___
#define avm_h___

#include <assert.h>
#include "jstypes.h"

namespace avmplus
{
#define FASTCALL

#define AvmAssert(x) assert(x)

    typedef JSUint32 uint32_t;
    
    class GC {
    };
    
    class GCObject {
    };
    
    













    enum ListElementType { LIST_NonGCObjects, LIST_GCObjects };

    template <typename T, ListElementType kElementType>
    class List
    {
    public:
        enum { kInitialCapacity = 128 };        

        List(GC *_gc, uint32_t _capacity=kInitialCapacity) : data(NULL), len(0)
        {
            ensureCapacity(_capacity);
            
            AvmAssert(sizeof(T) >= sizeof(void*));
        }
        
        ~List()
        {
            
            destroy();
            
            len = 0;
        }

        inline void destroy()
        {
            if (data)
                delete data;
        }
        
        uint32_t FASTCALL add(T value)
        {
            if (len >= capacity()) {
                grow();
            }
            wb(len++, value);
            return len-1;
        }
        
        inline bool isEmpty() const
        {
            return len == 0;
        }
        
        inline uint32_t size() const
        {
            return len;
        }
        
        inline uint32_t capacity() const
        {
            return data ? lot_size(data) / sizeof(T) : 0;
        }
        
        inline T get(uint32_t index) const
        {
            AvmAssert(index < len);
            return *(T*)lot_get(data, factor(index));
        }
        
        void FASTCALL set(uint32_t index, T value)
        {
            AvmAssert(index < capacity());
            if (index >= len)
            {
                len = index+1;
            }
            AvmAssert(len <= capacity());
            wb(index, value);
        }
        
        inline void clear()
        {
            zero_range(0, len);
            len = 0;
        }

        int FASTCALL indexOf(T value) const
        {
            for(uint32_t i=0; i<len; i++)
                if (get(i) == value)
                    return i;
            return -1;
        }
        
        int FASTCALL lastIndexOf(T value) const
        {
            for(int32_t i=len-1; i>=0; i--)
                if (get(i) == value)
                    return i;
            return -1;
        }   
        
        inline T last()
        {
            return get(len-1);
        }
        
        T FASTCALL removeLast()  
        { 
            if(isEmpty())
                return undef_list_val();
            T t = get(len-1);
            set(len-1, undef_list_val());
            len--;
            return t;
        }
    
        inline T operator[](uint32_t index) const
        {
            AvmAssert(index < capacity());
            return get(index);
        }
        
        void FASTCALL ensureCapacity(uint32_t cap)
        {           
            if (cap > capacity()) {
                if (data == NULL) {
                    data = new T[cap];
                    zero_range(0, cap);
                } else {
                    data = (T*)realloc(data, factor(cap));
                    zero_range(capacity(), cap - capacity());
                }
            }
        }
        
        void FASTCALL insert(uint32_t index, T value, uint32_t count = 1)
        {
            AvmAssert(index <= len);
            AvmAssert(count > 0);
            ensureCapacity(len+count);
            memmove(data + index + count, data + index, factor(len - index));
            wbzm(index, index+count, value);
            len += count;
        }

        T FASTCALL removeAt(uint32_t index)
        {
            T old = get(index);
            
            wb(index, undef_list_val());
            memmove(data + index, data + index + 1, factor(len - index - 1));
            len--;
            return old;
        }
    
    private:
        void FASTCALL grow()
        {
            
            uint32_t newMax = 0;
            const uint32_t curMax = capacity();
            if (curMax == 0)
                newMax = kInitialCapacity;
            else if(curMax > 15)
                newMax = curMax * 3/2;
            else
                newMax = curMax * 2;
        
            ensureCapacity(newMax);
        }
        
        inline void do_wb_nongc(void* , T* slot, T value)
        {   
            *slot = value;
        }

        inline void do_wb_gc(void* container, GCObject** slot, const GCObject** value)
        {   
            *slot = *value;
        }

        void FASTCALL wb(uint32_t index, T value)
        {   
            AvmAssert(index < capacity());
            AvmAssert(data != NULL);
            T* slot = data[index];
            switch(kElementType)
            {
                case LIST_NonGCObjects:
                    do_wb_nongc(0, slot, value);
                    break;
                case LIST_GCObjects:
                    do_wb_gc(0, (GCObject**)slot, (const GCObject**)&value);
                    break;
            }
        }

        
        
        
        
        void FASTCALL wbzm(uint32_t index, uint32_t index_end, T value)
        {   
            AvmAssert(index < capacity());
            AvmAssert(index_end <= capacity());
            AvmAssert(index < index_end);
            AvmAssert(data != NULL);
            void *container;
            T* slot = data + index;
            switch(kElementType)
            {
                case LIST_NonGCObjects:
                    for (  ; index < index_end; ++index, ++slot)
                        do_wb_nongc(container, slot, value);
                    break;
                case LIST_GCObjects:
                    for (  ; index < index_end; ++index, ++slot)
                        do_wb_gc(container, (GCObject**)slot, (const GCObject**)&value);
                    break;
            }
        }
        
        inline uint32_t factor(uint32_t index) const
        {
            return index * sizeof(T);
        }

        void FASTCALL zero_range(uint32_t _first, uint32_t _count)
        {
            memset(data + _first, 0, factor(_count));
        }
        
        
        static inline T undef_list_val();

    private:
        List(const List& toCopy);           
        void operator=(const List& that);   

    
    private:
        T* data;
        uint32_t len;
    

    };

    
    template<typename T, ListElementType kElementType> 
     inline T List<T, kElementType>::undef_list_val() { return T(0); }

    











    template <class K, class T, ListElementType valType>
    class SortedMap 
    {
    public:
        enum { kInitialCapacity= 64 };
        
        SortedMap(GC* gc, int _capacity=kInitialCapacity)
          : keys(gc, _capacity), values(gc, _capacity)
        {
        }

        bool isEmpty() const
        {
            return keys.size() == 0;
        }
        
        int size() const
        {
            return keys.size();
        }
        
        void clear()
        {
            keys.clear();
            values.clear();
        }
        
        void destroy()
        {
            keys.destroy();
            values.destroy();
        }
        
        T put(K k, T v)
        {
            if (keys.size() == 0 || k > keys.last()) 
            {
                keys.add(k);
                values.add(v);
                return (T)v;
            }
            else
            {
                int i = find(k);        
                if (i >= 0)
                {
                    T old = values[i];
                    keys.set(i, k);
                    values.set(i, v);
                    return old;
                }
                else
                {
                    i = -i - 1; 
                    AvmAssert(keys.size() != (uint32_t)i);
                    keys.insert(i, k);
                    values.insert(i, v);
                    return v;
                }
            }
        }
        
        T get(K k) const
        {
            int i = find(k);
            return i >= 0 ? values[i] : 0;
        }
        
        bool get(K k, T& v) const
        {
            int i = find(k);
            if (i >= 0)
            {
                v = values[i];
                return true;
            }
            return false;
        }
        
        bool containsKey(K k) const
        {
            int i = find(k);
            return (i >= 0) ? true : false;
        }
        
        T remove(K k)
        {
            int i = find(k);
            return removeAt(i);
        }
        
        T removeAt(int i)
        {
            T old = values.removeAt(i);
            keys.removeAt(i);
            return old;
        }

        T removeFirst() { return isEmpty() ? (T)0 : removeAt(0); }
        T removeLast()  { return isEmpty() ? (T)0 : removeAt(keys.size()-1); }
        T first() const { return isEmpty() ? (T)0 : values[0]; }
        T last()  const { return isEmpty() ? (T)0 : values[keys.size()-1]; }

        K firstKey() const  { return isEmpty() ? 0 : keys[0]; }
        K lastKey() const   { return isEmpty() ? 0 : keys[keys.size()-1]; }

        
        T   at(int i) const { return values[i]; }
        K   keyAt(int i) const { return keys[i]; }

        int findNear(K k) const {
            int i = find(k);
            return i >= 0 ? i : -i-2;
        }
    protected:
        List<K, LIST_NonGCObjects> keys;
        List<T, valType> values;
        
        int find(K k) const
        {
            int lo = 0;
            int hi = keys.size()-1;

            while (lo <= hi)
            {
                int i = (lo + hi)/2;
                K m = keys[i];
                if (k > m)
                    lo = i + 1;
                else if (k < m)
                    hi = i - 1;
                else
                    return i; 
            }
            return -(lo + 1);  
        }
    };
}

#endif