






#ifndef GrBackendEffectFactory_DEFINED
#define GrBackendEffectFactory_DEFINED

#include "GrTypes.h"
#include "SkTemplates.h"
#include "SkThread_platform.h"
#include "GrNoncopyable.h"











class GrEffectRef;
class GrGLEffect;
class GrGLCaps;
class GrDrawEffect;

class GrBackendEffectFactory : public GrNoncopyable {
public:
    typedef uint32_t EffectKey;
    enum {
        kNoEffectKey = 0,
        kEffectKeyBits = 15,
        





        kTextureKeyBits = 6,
        kAttribKeyBits = 6
    };

    virtual EffectKey glEffectKey(const GrDrawEffect&, const GrGLCaps&) const = 0;
    virtual GrGLEffect* createGLInstance(const GrDrawEffect&) const = 0;

    bool operator ==(const GrBackendEffectFactory& b) const {
        return fEffectClassID == b.fEffectClassID;
    }
    bool operator !=(const GrBackendEffectFactory& b) const {
        return !(*this == b);
    }

    virtual const char* name() const = 0;

protected:
    enum {
        kIllegalEffectClassID = 0,
    };

    GrBackendEffectFactory() {
        fEffectClassID = kIllegalEffectClassID;
    }
    virtual ~GrBackendEffectFactory() {}

    static EffectKey GenID() {
        GR_DEBUGCODE(static const int32_t kClassIDBits = 8 * sizeof(EffectKey) -
                           kTextureKeyBits - kEffectKeyBits - kAttribKeyBits);
        
        
        
        int32_t id = sk_atomic_inc(&fCurrEffectClassID) + 1;
        GrAssert(id < (1 << kClassIDBits));
        return static_cast<EffectKey>(id);
    }

    EffectKey fEffectClassID;

private:
    static int32_t fCurrEffectClassID;
};

#endif
