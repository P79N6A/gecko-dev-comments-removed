






































#ifndef __nanojit_Allocator__
#define __nanojit_Allocator__

namespace nanojit
{
    









    class Allocator {
    public:
        Allocator();
        ~Allocator();

        
        
        static const size_t MIN_CHUNK_SZB = 2000;

        void reset();

        
        void* alloc(size_t nbytes) {
            void* p;
            nbytes = (nbytes + 7) & ~7; 
            if (current_top + nbytes <= current_limit) {
                p = current_top;
                current_top += nbytes;
            } else {
                p = allocSlow(nbytes, false);
                NanoAssert(p);
            }
            return p;
        }

        
        void* fallibleAlloc(size_t nbytes) {
            void* p;
            nbytes = (nbytes + 7) & ~7; 
            if (current_top + nbytes <= current_limit) {
                p = current_top;
                current_top += nbytes;
            } else {
                p = allocSlow(nbytes, true);
            }
            return p;
        }

    protected:
        void* allocSlow(size_t nbytes, bool fallible = false);
        bool fill(size_t minbytes, bool fallible);

        class Chunk {
        public:
            Chunk* prev;
            int64_t data[1]; 
        };

        Chunk* current_chunk;
        char* current_top;
        char* current_limit;

        

        
        void* allocChunk(size_t nbytes, bool fallible);

        
        void freeChunk(void*);

        
        void postReset();
    };
}


inline void* operator new(size_t size, nanojit::Allocator &a) {
    return a.alloc(size);
}


inline void* operator new(size_t size, nanojit::Allocator *a) {
    return a->alloc(size);
}


inline void* operator new[](size_t size, nanojit::Allocator& a) {
    return a.alloc(size);
}


inline void* operator new[](size_t size, nanojit::Allocator* a) {
    return a->alloc(size);
}

#endif 
