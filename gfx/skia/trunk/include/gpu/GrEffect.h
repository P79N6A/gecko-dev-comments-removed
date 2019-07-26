






#ifndef GrEffect_DEFINED
#define GrEffect_DEFINED

#include "GrColor.h"
#include "GrEffectUnitTest.h"
#include "GrTexture.h"
#include "GrTextureAccess.h"
#include "GrTypesPriv.h"

class GrBackendEffectFactory;
class GrContext;
class GrCoordTransform;
class GrEffect;
class GrVertexEffect;
class SkString;












class GrEffectRef : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrEffectRef);
    virtual ~GrEffectRef();

    GrEffect* get() { return fEffect; }
    const GrEffect* get() const { return fEffect; }

    const GrEffect* operator-> () { return fEffect; }
    const GrEffect* operator-> () const { return fEffect; }

    void* operator new(size_t size);
    void operator delete(void* target);

    void* operator new(size_t size, void* placement) {
        return ::operator new(size, placement);
    }
    void operator delete(void* target, void* placement) {
        ::operator delete(target, placement);
    }

private:
    friend class GrEffect; 

    explicit GrEffectRef(GrEffect* effect);

    GrEffect* fEffect;

    typedef SkRefCnt INHERITED;
};


















class GrEffect : private SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrEffect)

    virtual ~GrEffect();

    






    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const = 0;

    
    bool willUseInputColor() const { return fWillUseInputColor; }

    














    virtual const GrBackendEffectFactory& getFactory() const = 0;

    








    bool isEqual(const GrEffectRef& other) const {
        return this->isEqual(*other.get());
    }

    

    const char* name() const;

    int numTransforms() const { return fCoordTransforms.count(); }

    

    const GrCoordTransform& coordTransform(int index) const { return *fCoordTransforms[index]; }

    int numTextures() const { return fTextureAccesses.count(); }

    

    const GrTextureAccess& textureAccess(int index) const { return *fTextureAccesses[index]; }

    
    GrTexture* texture(int index) const { return this->textureAccess(index).getTexture(); }

    
    bool willReadDstColor() const { return fWillReadDstColor; }

    
    bool willReadFragmentPosition() const { return fWillReadFragmentPosition; }

    

    bool hasVertexCode() const { return fHasVertexCode; }

    int numVertexAttribs() const {
        SkASSERT(0 == fVertexAttribTypes.count() || fHasVertexCode);
        return fVertexAttribTypes.count();
    }

    GrSLType vertexAttribType(int index) const { return fVertexAttribTypes[index]; }

    static const int kMaxVertexAttribs = 2;

    

    static inline SkMatrix MakeDivByTextureWHMatrix(const GrTexture* texture) {
        SkASSERT(NULL != texture);
        SkMatrix mat;
        mat.setIDiv(texture->width(), texture->height());
        return mat;
    }

    void* operator new(size_t size);
    void operator delete(void* target);

    void* operator new(size_t size, void* placement) {
        return ::operator new(size, placement);
    }
    void operator delete(void* target, void* placement) {
        ::operator delete(target, placement);
    }

    


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
    






    void addCoordTransform(const GrCoordTransform* coordTransform);

    





    void addTextureAccess(const GrTextureAccess* textureAccess);

    GrEffect()
        : fWillReadDstColor(false)
        , fWillReadFragmentPosition(false)
        , fWillUseInputColor(true)
        , fHasVertexCode(false)
        , fEffectRef(NULL) {}

    

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

    
    static GrEffectRef* CreateStaticEffectRef(void* refStorage, GrEffect* effect) {
        SkASSERT(NULL == effect->fEffectRef);
        effect->fEffectRef = SkNEW_PLACEMENT_ARGS(refStorage, GrEffectRef, (effect));
        return effect->fEffectRef;
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

    




    void setWillReadDstColor() { fWillReadDstColor = true; }

    




    void setWillReadFragmentPosition() { fWillReadFragmentPosition = true; }

    




    void setWillNotUseInputColor() { fWillUseInputColor = false; }

private:
    bool isEqual(const GrEffect& other) const {
        if (&this->getFactory() != &other.getFactory()) {
            return false;
        }
        bool result = this->onIsEqual(other);
#ifdef SK_DEBUG
        if (result) {
            this->assertEquality(other);
        }
#endif
        return result;
    }

    SkDEBUGCODE(void assertEquality(const GrEffect& other) const;)

    


    virtual bool onIsEqual(const GrEffect& other) const = 0;

    void EffectRefDestroyed() { fEffectRef = NULL; }

    friend class GrEffectRef;    
    friend class GrEffectStage;  
                                 
                                 
    friend class GrVertexEffect; 

    SkSTArray<4, const GrCoordTransform*, true>  fCoordTransforms;
    SkSTArray<4, const GrTextureAccess*, true>   fTextureAccesses;
    SkSTArray<kMaxVertexAttribs, GrSLType, true> fVertexAttribTypes;
    bool                                         fWillReadDstColor;
    bool                                         fWillReadFragmentPosition;
    bool                                         fWillUseInputColor;
    bool                                         fHasVertexCode;
    GrEffectRef*                                 fEffectRef;

    typedef SkRefCnt INHERITED;
};

inline GrEffectRef::GrEffectRef(GrEffect* effect) {
    SkASSERT(NULL != effect);
    effect->ref();
    fEffect = effect;
}





#define GR_CREATE_STATIC_EFFECT(NAME, EFFECT_CLASS, ARGS)                                         \
enum {                                                                                            \
    k_##NAME##_EffectRefOffset = GR_CT_ALIGN_UP(sizeof(EFFECT_CLASS), 8),                         \
    k_##NAME##_StorageSize = k_##NAME##_EffectRefOffset + sizeof(GrEffectRef)                     \
};                                                                                                \
static SkAlignedSStorage<k_##NAME##_StorageSize> g_##NAME##_Storage;                              \
static void* NAME##_RefLocation = (char*)g_##NAME##_Storage.get() + k_##NAME##_EffectRefOffset;   \
static GrEffect* NAME##_Effect SkNEW_PLACEMENT_ARGS(g_##NAME##_Storage.get(), EFFECT_CLASS, ARGS);\
static SkAutoTDestroy<GrEffect> NAME##_ad(NAME##_Effect);                                         \
static GrEffectRef* NAME(GrEffect::CreateStaticEffectRef(NAME##_RefLocation, NAME##_Effect));     \
static SkAutoTDestroy<GrEffectRef> NAME##_Ref_ad(NAME)


#endif
