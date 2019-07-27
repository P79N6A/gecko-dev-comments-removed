






#ifndef GrBackendEffectFactory_DEFINED
#define GrBackendEffectFactory_DEFINED

#include "GrTypes.h"
#include "SkTemplates.h"
#include "SkThread.h"
#include "SkTypes.h"
#include "SkTArray.h"

class GrGLEffect;
class GrGLCaps;
class GrDrawEffect;




class GrEffectKeyBuilder {
public:
    GrEffectKeyBuilder(SkTArray<unsigned char, true>* data) : fData(data), fCount(0) {
        SkASSERT(0 == fData->count() % sizeof(uint32_t));
    }

    void add32(uint32_t v) {
        ++fCount;
        fData->push_back_n(4, reinterpret_cast<uint8_t*>(&v));
    }

    

    uint32_t* SK_WARN_UNUSED_RESULT add32n(int count) {
        SkASSERT(count > 0);
        fCount += count;
        return reinterpret_cast<uint32_t*>(fData->push_back_n(4 * count));
    }

    size_t size() const { return sizeof(uint32_t) * fCount; }

private:
    SkTArray<uint8_t, true>* fData; 
    int fCount;                     
};






class GrEffectKey {
public:
    GrEffectKey(const uint32_t* key, int count) : fKey(key), fCount(count) {
        SkASSERT(0 == reinterpret_cast<intptr_t>(key) % sizeof(uint32_t));
    }

    
    uint32_t get32(int index) const {
        SkASSERT(index >=0 && index < fCount);
        return fKey[index];
    }

    
    int count32() const { return fCount; }

private:
    const uint32_t* fKey;           
    int             fCount;         
};


















class GrBackendEffectFactory : SkNoncopyable {
public:
    




    virtual void getGLEffectKey(const GrDrawEffect&, const GrGLCaps&, GrEffectKeyBuilder*) const = 0;

    



    virtual GrGLEffect* createGLInstance(const GrDrawEffect&) const = 0;

    


    virtual const char* name() const = 0;

    




    uint32_t effectClassID() const { return fEffectClassID; }

protected:
    GrBackendEffectFactory() : fEffectClassID(GenID()) {}
    virtual ~GrBackendEffectFactory() {}

private:
    enum {
        kIllegalEffectClassID = 0,
    };

    static uint32_t GenID() {
        
        
        
        uint32_t id = static_cast<uint32_t>(sk_atomic_inc(&fCurrEffectClassID)) + 1;
        if (!id) {
            SkFAIL("This should never wrap as it should only be called once for each GrEffect "
                   "subclass.");
        }
        return id;
    }

    const uint32_t fEffectClassID;
    static int32_t fCurrEffectClassID;
};

#endif
