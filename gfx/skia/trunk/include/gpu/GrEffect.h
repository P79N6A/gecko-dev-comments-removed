






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











class GrEffect : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrEffect)

    virtual ~GrEffect();

    






    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const = 0;

    
    bool willUseInputColor() const { return fWillUseInputColor; }

    














    virtual const GrBackendEffectFactory& getFactory() const = 0;

    







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

protected:
    






    void addCoordTransform(const GrCoordTransform* coordTransform);

    





    void addTextureAccess(const GrTextureAccess* textureAccess);

    GrEffect()
        : fWillReadDstColor(false)
        , fWillReadFragmentPosition(false)
        , fWillUseInputColor(true)
        , fHasVertexCode(false) {}

    


    template <typename T> static const T& CastEffect(const GrEffect& effect) {
        return *static_cast<const T*>(&effect);
    }

    




    void setWillReadDstColor() { fWillReadDstColor = true; }

    




    void setWillReadFragmentPosition() { fWillReadFragmentPosition = true; }

    




    void setWillNotUseInputColor() { fWillUseInputColor = false; }

private:
    SkDEBUGCODE(void assertEquality(const GrEffect& other) const;)

    


    virtual bool onIsEqual(const GrEffect& other) const = 0;

    friend class GrVertexEffect; 

    SkSTArray<4, const GrCoordTransform*, true>  fCoordTransforms;
    SkSTArray<4, const GrTextureAccess*, true>   fTextureAccesses;
    SkSTArray<kMaxVertexAttribs, GrSLType, true> fVertexAttribTypes;
    bool                                         fWillReadDstColor;
    bool                                         fWillReadFragmentPosition;
    bool                                         fWillUseInputColor;
    bool                                         fHasVertexCode;

    typedef SkRefCnt INHERITED;
};





#define GR_CREATE_STATIC_EFFECT(NAME, EFFECT_CLASS, ARGS)                                         \
static SkAlignedSStorage<sizeof(EFFECT_CLASS)> g_##NAME##_Storage;                                \
static GrEffect* NAME SkNEW_PLACEMENT_ARGS(g_##NAME##_Storage.get(), EFFECT_CLASS, ARGS);         \
static SkAutoTDestroy<GrEffect> NAME##_ad(NAME);


#endif
