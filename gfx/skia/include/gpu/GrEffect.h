






#ifndef GrEffect_DEFINED
#define GrEffect_DEFINED

#include "GrColor.h"
#include "GrEffectUnitTest.h"
#include "GrNoncopyable.h"
#include "GrRefCnt.h"
#include "GrTexture.h"
#include "GrTextureAccess.h"
#include "GrTypesPriv.h"

class GrBackendEffectFactory;
class GrContext;
class GrEffect;
class SkString;








class GrEffectRef : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrEffectRef);

    GrEffect* get() { return fEffect; }
    const GrEffect* get() const { return fEffect; }

    const GrEffect* operator-> () { return fEffect; }
    const GrEffect* operator-> () const { return fEffect; }

    void* operator new(size_t size);
    void operator delete(void* target);

private:
    friend class GrEffect; 

    explicit GrEffectRef(GrEffect* effect);

    virtual ~GrEffectRef();

    GrEffect* fEffect;

    typedef SkRefCnt INHERITED;
};














class GrEffect : private GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrEffect)

    






    enum CoordsType {
        kLocal_CoordsType,
        kPosition_CoordsType,

        kCustom_CoordsType,
    };

    virtual ~GrEffect();

    






    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const = 0;

    














    virtual const GrBackendEffectFactory& getFactory() const = 0;

    








    bool isEqual(const GrEffectRef& other) const {
        return this->isEqual(*other.get());
    }

    

    const char* name() const;

    int numTextures() const { return fTextureAccesses.count(); }

    

    const GrTextureAccess& textureAccess(int index) const { return *fTextureAccesses[index]; }

    
    GrTexture* texture(int index) const { return this->textureAccess(index).getTexture(); }

    
    bool willReadDst() const { return fWillReadDst; }

    int numVertexAttribs() const { return fVertexAttribTypes.count(); }

    GrSLType vertexAttribType(int index) const { return fVertexAttribTypes[index]; }

    static const int kMaxVertexAttribs = 2;

    

    static inline SkMatrix MakeDivByTextureWHMatrix(const GrTexture* texture) {
        GrAssert(NULL != texture);
        SkMatrix mat;
        mat.setIDiv(texture->width(), texture->height());
        return mat;
    }

    void* operator new(size_t size);
    void operator delete(void* target);

    


    void incDeferredRefCounts() const {
        this->ref();
        int count = fTextureAccesses.count();
        for (int t = 0; t < count; ++t) {
            fTextureAccesses[t]->getTexture()->incDeferredRefCount();
        }
    }
    void decDeferredRefCounts() const {
        int count = fTextureAccesses.count();
        for (int t = 0; t < count; ++t) {
            fTextureAccesses[t]->getTexture()->decDeferredRefCount();
        }
        this->unref();
    }

protected:
    




    void addTextureAccess(const GrTextureAccess* textureAccess);

    




    void addVertexAttrib(GrSLType type);

    GrEffect() : fWillReadDst(false), fEffectRef(NULL) {}

    

    static GrEffectRef* CreateEffectRef(GrEffect* effect) {
        if (NULL == effect->fEffectRef) {
            effect->fEffectRef = SkNEW_ARGS(GrEffectRef, (effect));
        } else {
            effect->fEffectRef->ref();
        }
        return effect->fEffectRef;
    }

    static const GrEffectRef* CreateEffectRef(const GrEffect* effect) {
        return CreateEffectRef(const_cast<GrEffect*>(effect));
    }

    









    class AutoEffectUnref {
    public:
        AutoEffectUnref(GrEffect* effect) : fEffect(effect) { }
        ~AutoEffectUnref() { fEffect->unref(); }
        operator GrEffect*() { return fEffect; }
    private:
        GrEffect* fEffect;
    };

    

    template <typename T>
    static const T& CastEffect(const GrEffect& effectRef) {
        return *static_cast<const T*>(&effectRef);
    }

    




    void setWillReadDst() { fWillReadDst = true; }

private:
    bool isEqual(const GrEffect& other) const {
        if (&this->getFactory() != &other.getFactory()) {
            return false;
        }
        bool result = this->onIsEqual(other);
#if GR_DEBUG
        if (result) {
            GrAssert(this->numTextures() == other.numTextures());
            for (int i = 0; i < this->numTextures(); ++i) {
                GrAssert(*fTextureAccesses[i] == *other.fTextureAccesses[i]);
            }
        }
#endif
        return result;
    }

    


    virtual bool onIsEqual(const GrEffect& other) const = 0;

    void EffectRefDestroyed() { fEffectRef = NULL; }

    friend class GrEffectRef;   
    friend class GrEffectStage; 
                                
                                

    SkSTArray<4, const GrTextureAccess*, true>   fTextureAccesses;
    SkSTArray<kMaxVertexAttribs, GrSLType, true> fVertexAttribTypes;
    bool                                         fWillReadDst;
    GrEffectRef*                                 fEffectRef;

    typedef GrRefCnt INHERITED;
};

inline GrEffectRef::GrEffectRef(GrEffect* effect) {
    GrAssert(NULL != effect);
    effect->ref();
    fEffect = effect;
}

#endif
