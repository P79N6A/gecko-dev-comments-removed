








#ifndef SkMetaData_DEFINED
#define SkMetaData_DEFINED

#include "SkScalar.h"

class SkRefCnt;

class SK_API SkMetaData {
public:
    
















    typedef void* (*PtrProc)(void* ptr, bool doRef);

    


    static void* RefCntProc(void* ptr, bool doRef);

    SkMetaData();
    SkMetaData(const SkMetaData& src);
    ~SkMetaData();

    SkMetaData& operator=(const SkMetaData& src);

    void reset();

    bool findS32(const char name[], int32_t* value = NULL) const;
    bool findScalar(const char name[], SkScalar* value = NULL) const;
    const SkScalar* findScalars(const char name[], int* count,
                                SkScalar values[] = NULL) const;
    const char* findString(const char name[]) const;
    bool findPtr(const char name[], void** value = NULL, PtrProc* = NULL) const;
    bool findBool(const char name[], bool* value = NULL) const;
    const void* findData(const char name[], size_t* byteCount = NULL) const;

    bool hasS32(const char name[], int32_t value) const {
        int32_t v;
        return this->findS32(name, &v) && v == value;
    }
    bool hasScalar(const char name[], SkScalar value) const {
        SkScalar v;
        return this->findScalar(name, &v) && v == value;
    }
    bool hasString(const char name[], const char value[]) const {
        const char* v = this->findString(name);
        return  (v == NULL && value == NULL) ||
                (v != NULL && value != NULL && !strcmp(v, value));
    }
    bool hasPtr(const char name[], void* value) const {
        void* v;
        return this->findPtr(name, &v) && v == value;
    }
    bool hasBool(const char name[], bool value) const {
        bool    v;
        return this->findBool(name, &v) && v == value;
    }
    bool hasData(const char name[], const void* data, size_t byteCount) const {
        size_t len;
        const void* ptr = this->findData(name, &len);
        return NULL != ptr && len == byteCount && !memcmp(ptr, data, len);
    }

    void setS32(const char name[], int32_t value);
    void setScalar(const char name[], SkScalar value);
    SkScalar* setScalars(const char name[], int count, const SkScalar values[] = NULL);
    void setString(const char name[], const char value[]);
    void setPtr(const char name[], void* value, PtrProc proc = NULL);
    void setBool(const char name[], bool value);
    
    void setData(const char name[], const void* data, size_t byteCount);

    bool removeS32(const char name[]);
    bool removeScalar(const char name[]);
    bool removeString(const char name[]);
    bool removePtr(const char name[]);
    bool removeBool(const char name[]);
    bool removeData(const char name[]);

    
    bool findRefCnt(const char name[], SkRefCnt** ptr = NULL) {
        return this->findPtr(name, reinterpret_cast<void**>(ptr));
    }
    bool hasRefCnt(const char name[], SkRefCnt* ptr) {
        return this->hasPtr(name, ptr);
    }
    void setRefCnt(const char name[], SkRefCnt* ptr) {
        this->setPtr(name, ptr, RefCntProc);
    }
    bool removeRefCnt(const char name[]) {
        return this->removePtr(name);
    }

    enum Type {
        kS32_Type,
        kScalar_Type,
        kString_Type,
        kPtr_Type,
        kBool_Type,
        kData_Type,

        kTypeCount
    };

    struct Rec;
    class Iter;
    friend class Iter;

    class Iter {
    public:
        Iter() : fRec(NULL) {}
        Iter(const SkMetaData&);

        


        void reset(const SkMetaData&);

        




        const char* next(Type*, int* count);

    private:
        Rec* fRec;
    };

public:
    struct Rec {
        Rec*        fNext;
        uint16_t    fDataCount; 
        uint8_t     fDataLen;   
        uint8_t     fType;

        const void* data() const { return (this + 1); }
        void*       data() { return (this + 1); }
        const char* name() const { return (const char*)this->data() + fDataLen * fDataCount; }
        char*       name() { return (char*)this->data() + fDataLen * fDataCount; }

        static Rec* Alloc(size_t);
        static void Free(Rec*);
    };
    Rec*    fRec;

    const Rec* find(const char name[], Type) const;
    void* set(const char name[], const void* data, size_t len, Type, int count);
    bool remove(const char name[], Type);
};

#endif

