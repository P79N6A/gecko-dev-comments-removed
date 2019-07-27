






#ifndef SkRecord_DEFINED
#define SkRecord_DEFINED

#include "SkChunkAlloc.h"
#include "SkRecords.h"
#include "SkTLogic.h"
#include "SkTemplates.h"













class SkRecord : SkNoncopyable {
public:
    SkRecord(size_t chunkBytes = 4096, unsigned firstReserveCount = 64 / sizeof(void*))
        : fAlloc(chunkBytes), fCount(0), fReserved(0), kFirstReserveCount(firstReserveCount) {}

    ~SkRecord() {
        Destroyer destroyer;
        for (unsigned i = 0; i < this->count(); i++) {
            this->mutate<void>(i, destroyer);
        }
    }

    
    unsigned count() const { return fCount; }

    
    
    
    
    template <typename R, typename F>
    R visit(unsigned i, F& f) const {
        SkASSERT(i < this->count());
        return fRecords[i].visit<R>(fTypes[i], f);
    }

    
    
    
    
    template <typename R, typename F>
    R mutate(unsigned i, F& f) {
        SkASSERT(i < this->count());
        return fRecords[i].mutate<R>(fTypes[i], f);
    }
    

    
    
    template <typename T>
    T* alloc(unsigned count = 1) {
        return (T*)fAlloc.allocThrow(sizeof(T) * count);
    }

    
    
    template <typename T>
    T* append() {
        if (fCount == fReserved) {
            fReserved = SkTMax(kFirstReserveCount, fReserved*2);
            fRecords.realloc(fReserved);
            fTypes.realloc(fReserved);
        }

        fTypes[fCount] = T::kType;
        return fRecords[fCount++].set(this->allocCommand<T>());
    }

    
    
    
    template <typename T>
    T* replace(unsigned i) {
        SkASSERT(i < this->count());

        Destroyer destroyer;
        this->mutate<void>(i, destroyer);

        fTypes[i] = T::kType;
        return fRecords[i].set(this->allocCommand<T>());
    }

    
    
    
    template <typename T, typename Existing>
    T* replace(unsigned i, const SkRecords::Adopted<Existing>& proofOfAdoption) {
        SkASSERT(i < this->count());

        SkASSERT(Existing::kType == fTypes[i]);
        SkASSERT(proofOfAdoption == fRecords[i].ptr<Existing>());

        fTypes[i] = T::kType;
        return fRecords[i].set(this->allocCommand<T>());
    }

private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    struct Destroyer {
        template <typename T>
        void operator()(T* record) { record->~T(); }
    };

    
    struct Type8 {
    public:
        
        Type8(SkRecords::Type type) : fType(type) { SkASSERT(*this == type); }
        operator SkRecords::Type () { return (SkRecords::Type)fType; }

    private:
        uint8_t fType;
    };

    
    
    template <typename T>
    SK_WHEN(SkTIsEmpty<T>, T*) allocCommand() {
        static T singleton = {};
        return &singleton;
    }

    template <typename T>
    SK_WHEN(!SkTIsEmpty<T>, T*) allocCommand() { return this->alloc<T>(); }

    
    
    struct Record {
    public:
        
        template <typename T>
        T* set(T* ptr) {
            fPtr = ptr;
            return ptr;
        }

        
        template <typename T>
        T* ptr() const { return (T*)fPtr; }

        
        
        template <typename R, typename F>
        R visit(Type8 type, F& f) const {
        #define CASE(T) case SkRecords::T##_Type: return f(*this->ptr<SkRecords::T>());
            switch(type) { SK_RECORD_TYPES(CASE) }
        #undef CASE
            SkDEBUGFAIL("Unreachable");
            return R();
        }

        
        
        template <typename R, typename F>
        R mutate(Type8 type, F& f) {
        #define CASE(T) case SkRecords::T##_Type: return f(this->ptr<SkRecords::T>());
            switch(type) { SK_RECORD_TYPES(CASE) }
        #undef CASE
            SkDEBUGFAIL("Unreachable");
            return R();
        }

    private:
        void* fPtr;
    };

    
    
    
    
    

    SkChunkAlloc fAlloc;
    SkAutoTMalloc<Record> fRecords;
    SkAutoTMalloc<Type8> fTypes;
    
    unsigned fCount;
    unsigned fReserved;
    const unsigned kFirstReserveCount;
};

#endif
