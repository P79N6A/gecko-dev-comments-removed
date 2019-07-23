






































#ifndef __nanojit_Allocator__
#define __nanojit_Allocator__

namespace nanojit
{
    







    class Allocator {
    public:
        Allocator();
        virtual ~Allocator();
        void reset();

        
        void* alloc(size_t nbytes) {
            nbytes = (nbytes + 7) & ~7; 
            if (current_top + nbytes <= current_limit) {
                void *p = current_top;
                current_top += nbytes;
                return p;
            }
            return allocSlow(nbytes);
        }

    private:
        void* allocSlow(size_t nbytes);
        void fill(size_t minbytes);

        class Chunk {
        public:
            Chunk* prev;
            int64_t data[1]; 
        };

        Chunk* current_chunk;
        char* current_top;
        char* current_limit;

    
    private:
        
        void* allocChunk(size_t nbytes);

        
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
