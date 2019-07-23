


































#ifndef avm_h___
#define avm_h___

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(AVMPLUS_UNIX)
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "jstypes.h"
#include "jsstdint.h"

#if !defined(AVMPLUS_LITTLE_ENDIAN) && !defined(AVMPLUS_BIG_ENDIAN)
#ifdef IS_BIG_ENDIAN
#define AVMPLUS_BIG_ENDIAN
#else
#define AVMPLUS_LITTLE_ENDIAN
#endif
#endif

#define FASTCALL JS_FASTCALL

#if defined(JS_NO_FASTCALL)
#define NJ_NO_FASTCALL
#if defined(AVMPLUS_IA32)
#define SIMULATE_FASTCALL(lr, state_ptr, frag_ptr, func_addr)   \
    asm volatile(                                               \
        "call *%%esi"                                           \
        : "=a" (lr)                                             \
        : "c" (state_ptr), "d" (frag_ptr), "S" (func_addr)      \
        : "memory", "cc"                                        \
    );
#endif 
#endif 

#ifdef WIN32
#include <windows.h>
#endif

#if defined(DEBUG) || defined(MOZ_NO_VARADIC_MACROS)
#if !defined _DEBUG
#define _DEBUG
#endif
#define NJ_VERBOSE 1
#define NJ_PROFILE 1
#include <stdarg.h>
#endif

#ifdef _DEBUG
void NanoAssertFail();
#endif

#define AvmAssert(x) assert(x)
#define AvmAssertMsg(x, y)
#define AvmDebugLog(x) printf x

#if defined(AVMPLUS_IA32)
#if defined(_MSC_VER)
__declspec(naked) static inline __int64 rdtsc()
{
    __asm
    {
        rdtsc;
        ret;
    }
}
#elif defined(SOLARIS)
static inline unsigned long long rdtsc(void)
{
    unsigned long long int x;
    asm volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}
#endif 

#elif defined(__x86_64__)

static __inline__ uint64_t rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

#elif defined(__powerpc__)

typedef unsigned long long int unsigned long long;

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int result=0;
  unsigned long int upper, lower,tmp;
  __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0           \n"
                "\tmftb    %1           \n"
                "\tmftbu   %2           \n"
                "\tcmpw    %2,%0        \n"
                "\tbne     0b         \n"
                : "=r"(upper),"=r"(lower),"=r"(tmp)
                );
  result = upper;
  result = result<<32;
  result = result|lower;

  return(result);
}

#endif 

struct JSContext;

namespace MMgc {

    class GC;

    class GCObject
    {
    public:
        inline void*
        operator new(size_t size, GC* gc)
        {
            return calloc(1, size);
        }

        static void operator delete (void *gcObject)
        {
            free(gcObject);
        }
    };

    #define MMGC_SUBCLASS_DECL : public avmplus::GCObject

    class GCFinalizedObject : public GCObject
    {
    public:
        static void operator delete (void *gcObject)
        {
            free(gcObject);
        }
    };

    class GCHeap
    {
    public:
        int32_t kNativePageSize;

        GCHeap()
        {
    #if defined _SC_PAGE_SIZE
            kNativePageSize = sysconf(_SC_PAGE_SIZE);
    #else
            kNativePageSize = 4096; 
    #endif
        }

        inline void*
        Alloc(uint32_t pages)
        {
    #ifdef XP_WIN
            return VirtualAlloc(NULL,
                                pages * kNativePageSize,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_EXECUTE_READWRITE);
    #elif defined AVMPLUS_UNIX
            



            return mmap(NULL,
                        pages * kNativePageSize,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANON,
                        -1,
                        0);
    #else
            return valloc(pages * kNativePageSize);
    #endif
        }

        inline void
        Free(void* p, uint32_t pages)
        {
    #ifdef XP_WIN
            VirtualFree(p, 0, MEM_RELEASE);
    #elif defined AVMPLUS_UNIX
            #if defined SOLARIS
            munmap((char*)p, pages * kNativePageSize);
            #else
            munmap(p, pages * kNativePageSize);
            #endif
    #else
            free(p);
    #endif
        }

    };

    class GC
    {
        static GCHeap heap;

    public:
        


        enum AllocFlags
        {
            kZero=1,
            kContainsPointers=2,
            kFinalize=4,
            kRCObject=8
        };

        static inline void*
        Alloc(uint32_t bytes, int flags=kZero)
        {
          if (flags & kZero)
            return calloc(1, bytes);
          else
            return malloc(bytes);
        }

        static inline void
        Free(void* p)
        {
            free(p);
        }

        static inline GCHeap*
        GetGCHeap()
        {
            return &heap;
        }
    };
}

#define DWB(x) x
#define DRCWB(x) x
#define WB(gc, container, addr, value) do { *(addr) = (value); } while(0)
#define WBRC(gc, container, addr, value) do { *(addr) = (value); } while(0)

#define MMGC_MEM_TYPE(x)

extern void VMPI_setPageProtection(void *address,
                                   size_t size,
                                   bool executableFlag,
                                   bool writeableFlag);

namespace avmplus {

    using namespace MMgc;

    typedef int FunctionID;

    extern void AvmLog(char const *msg, ...);

    class String
    {
    };

    typedef class String AvmString;

    class StringNullTerminatedUTF8
    {
        const char* cstr;

    public:
        StringNullTerminatedUTF8(GC* gc, String* s)
        {
            cstr = strdup((const char*)s);
        }

        ~StringNullTerminatedUTF8()
        {
            free((void*)cstr);
        }

        inline
        const char* c_str()
        {
            return cstr;
        }
    };

    typedef String* Stringp;

    class Config
    {
    public:
        Config() {
            memset(this, 0, sizeof(Config));
#ifdef DEBUG
            verbose = false;
            verbose_addrs = 1;
            verbose_exits = 1;
            verbose_live = 1;
            show_stats = 1;
#endif
        }

        uint32_t tree_opt:1;
        uint32_t quiet_opt:1;
        uint32_t verbose:1;
        uint32_t verbose_addrs:1;
        uint32_t verbose_live:1;
        uint32_t verbose_exits:1;
        uint32_t show_stats:1;

#if defined (AVMPLUS_IA32)
    
        bool sse2;
        bool use_cmov;
#endif

#if defined (AVMPLUS_ARM)
        
# if defined (NJ_FORCE_SOFTFLOAT)
        static const bool vfp = false;
# else
        bool vfp;
# endif

        
# if defined (NJ_FORCE_ARM_ARCH_VERSION)
        static const unsigned int arch = NJ_FORCE_ARM_ARCH_VERSION;
# else
        unsigned int arch;
# endif

        
        
# if defined (NJ_FORCE_NO_ARM_THUMB)
        static const bool thumb = false;
# else
        bool thumb;
# endif

        
        
# if defined (NJ_FORCE_NO_ARM_THUMB2)
        static const bool thumb2 = false;
# else
        bool thumb2;
# endif

#endif

#if defined (NJ_FORCE_SOFTFLOAT)
        static const bool soft_float = true;
#else
        bool soft_float;
#endif
    };

    static const int kstrconst_emptyString = 0;

    class AvmInterpreter
    {
        class Labels {
        public:
            const char* format(const void* ip)
            {
                static char buf[33];
                sprintf(buf, "%p", ip);
                return buf;
            }
        };

        Labels _labels;
    public:
        Labels* labels;

        AvmInterpreter()
        {
            labels = &_labels;
        }

    };

    class AvmConsole
    {
    public:
        AvmConsole& operator<<(const char* s)
        {
            fprintf(stdout, "%s", s);
            return *this;
        }
    };

    class AvmCore
    {
    public:
        AvmInterpreter interp;
        AvmConsole console;

        static Config config;
        static GC* gc;
        static String* k_str[];

#ifdef AVMPLUS_IA32
        static inline bool
        use_sse2()
        {
            return config.sse2;
        }
#endif

        static inline bool
        use_cmov()
        {
#ifdef AVMPLUS_IA32
            return config.use_cmov;
#else
        return true;
#endif
        }

        static inline bool
        quiet_opt()
        {
            return config.quiet_opt;
        }

        static inline bool
        verbose()
        {
            return config.verbose;
        }

        static inline GC*
        GetGC()
        {
            return gc;
        }

        static inline String* newString(const char* cstr) {
            return (String*)strdup(cstr);
        }

        static inline void freeString(String* str) {
            return free((char*)str);
        }
    };

    class OSDep
    {
    public:
        static inline void
        getDate()
        {
        }
    };

    













    enum ListElementType {
        LIST_NonGCObjects = 0,
        LIST_GCObjects = 1,
        LIST_RCObjects = 2
    };

    template <typename T, ListElementType kElementType>
    class List
    {
    public:
        enum { kInitialCapacity = 128 };

        List(GC *_gc, uint32_t _capacity=kInitialCapacity) : data(NULL), len(0), capacity(0)
        {
            ensureCapacity(_capacity);
        }

        ~List()
        {
            
            destroy();
            
            len = 0;
        }

        inline void destroy()
        {
            if (data)
                free(data);
        }

        const T *getData() const { return data; }

        
        void become(List& that)
        {
            this->destroy();

            this->data = that.data;
            this->len = that.len;
        this->capacity = that.capacity;

            that.data = 0;
            that.len = 0;
        that.capacity = 0;
        }
        uint32_t add(T value)
        {
            if (len >= capacity) {
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

        inline T get(uint32_t index) const
        {
            AvmAssert(index < len);
            return *(T*)(data + index);
        }

        void set(uint32_t index, T value)
        {
            AvmAssert(index < capacity);
            if (index >= len)
            {
                len = index+1;
            }
            AvmAssert(len <= capacity);
            wb(index, value);
        }

        void add(const List<T, kElementType>& l)
        {
            ensureCapacity(len+l.size());
            
            AvmAssert(kElementType != LIST_RCObjects);
            arraycopy(l.getData(), 0, data, len, l.size());
            len += l.size();
        }

        inline void clear()
        {
            zero_range(0, len);
            len = 0;
        }

        int indexOf(T value) const
        {
            for(uint32_t i=0; i<len; i++)
                if (get(i) == value)
                    return i;
            return -1;
        }

        int lastIndexOf(T value) const
        {
            for(int32_t i=len-1; i>=0; i--)
                if (get(i) == value)
                    return i;
            return -1;
        }

        inline T last() const
        {
            return get(len-1);
        }

        T removeLast()
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
            AvmAssert(index < capacity);
            return get(index);
        }

        void ensureCapacity(uint32_t cap)
        {
            if (cap > capacity) {
                if (data == NULL) {
                    data = (T*)calloc(1, factor(cap));
                } else {
                    data = (T*)realloc(data, factor(cap));
                    zero_range(capacity, cap - capacity);
                }
                capacity = cap;
            }
        }

        void insert(uint32_t index, T value, uint32_t count = 1)
        {
            AvmAssert(index <= len);
            AvmAssert(count > 0);
            ensureCapacity(len+count);
            memmove(data + index + count, data + index, factor(len - index));
            wbzm(index, index+count, value);
            len += count;
        }

        T removeAt(uint32_t index)
        {
            T old = get(index);
            
            wb(index, undef_list_val());
            memmove(data + index, data + index + 1, factor(len - index - 1));
            len--;
            return old;
        }

    private:
        void grow()
        {
            
            uint32_t newMax = 0;
            const uint32_t curMax = capacity;
            if (curMax == 0)
                newMax = kInitialCapacity;
            else if(curMax > 15)
                newMax = curMax * 3/2;
            else
                newMax = curMax * 2;

            ensureCapacity(newMax);
        }

        void arraycopy(const T* src, int srcStart, T* dst, int dstStart, int nbr)
        {
            
            if ((src == dst) && (srcStart > dstStart) )
            {
                for(int i=0; i<nbr; i++)
                    dst[i+dstStart] = src[i+srcStart];
            }
            else
            {
                for(int i=nbr-1; i>=0; i--)
                    dst[i+dstStart] = src[i+srcStart];
            }
        }

        inline void do_wb_nongc(T* slot, T value)
        {
            *slot = value;
        }

        inline void do_wb_gc(GCObject** slot, const GCObject** value)
        {
            *slot = (GCObject*)*value;
        }

        void wb(uint32_t index, T value)
        {
            AvmAssert(index < capacity);
            AvmAssert(data != NULL);
            T* slot = &data[index];
            do_wb_nongc(slot, value);
        }

        
        
        
        
        void wbzm(uint32_t index, uint32_t index_end, T value)
        {
            AvmAssert(index < capacity);
            AvmAssert(index_end <= capacity);
            AvmAssert(index < index_end);
            AvmAssert(data != NULL);
            T* slot = data + index;
            for (  ; index < index_end; ++index, ++slot)
                do_wb_nongc(slot, value);
        }

        inline uint32_t factor(uint32_t index) const
        {
            return index * sizeof(T);
        }

        void zero_range(uint32_t _first, uint32_t _count)
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
        uint32_t capacity;
    

    };

    
    template<typename T, ListElementType kElementType>
     inline T List<T, kElementType>::undef_list_val() { return T(0); }

    











    template <class K, class T, ListElementType valType>
    class SortedMap : public GCObject
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

    #define GCSortedMap SortedMap

    











    class BitSet
    {
        public:
            enum {  kUnit = 8*sizeof(long),
                    kDefaultCapacity = 4   };

            BitSet()
            {
                capacity = kDefaultCapacity;
                reset();
            }

            ~BitSet()
            {
                if (capacity > kDefaultCapacity)
                    free(bits.ptr);
            }

            void reset()
            {
                if (capacity > kDefaultCapacity)
                    for(int i=0; i<capacity; i++)
                        bits.ptr[i] = 0;
                else
                    for(int i=0; i<capacity; i++)
                        bits.ar[i] = 0;
            }

            void set(GC *gc, int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index >= capacity)
                    grow(gc, index+1);

                if (capacity > kDefaultCapacity)
                    bits.ptr[index] |= (1<<bit);
                else
                    bits.ar[index] |= (1<<bit);
            }

            void clear(int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index < capacity)
                {
                    if (capacity > kDefaultCapacity)
                        bits.ptr[index] &= ~(1<<bit);
                    else
                        bits.ar[index] &= ~(1<<bit);
                }
            }

            bool get(int bitNbr) const
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                bool value = false;
                if (index < capacity)
                {
                    if (capacity > kDefaultCapacity)
                        value = ( bits.ptr[index] & (1<<bit) ) ? true : false;
                    else
                        value = ( bits.ar[index] & (1<<bit) ) ? true : false;
                }
                return value;
            }

        private:
            
            void grow(GC *gc, int newCapacity)
            {
                
                newCapacity *= 2;
                
                long* newBits = (long*)calloc(1, newCapacity * sizeof(long));
                

                
                if (capacity > kDefaultCapacity)
                    for(int i=0; i<capacity; i++)
                        newBits[i] = bits.ptr[i];
                else
                    for(int i=0; i<capacity; i++)
                        newBits[i] = bits.ar[i];

                
                if (capacity > kDefaultCapacity)
                    free(bits.ptr);

                bits.ptr = newBits;
                capacity = newCapacity;
            }

            
            
            
            int capacity;
            union
            {
                long ar[kDefaultCapacity];
                long*  ptr;
            }
            bits;
    };
}

#endif
