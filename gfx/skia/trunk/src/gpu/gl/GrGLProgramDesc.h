






#ifndef GrGLProgramDesc_DEFINED
#define GrGLProgramDesc_DEFINED

#include "GrGLEffect.h"
#include "GrDrawState.h"
#include "GrGLShaderBuilder.h"

class GrGpuGL;

#ifdef SK_DEBUG
  
  
  #define GR_GL_EXPERIMENTAL_GS 1
#else
  #define GR_GL_EXPERIMENTAL_GS 0
#endif






class GrGLProgramDesc {
public:
    GrGLProgramDesc() : fInitialized(false) {}
    GrGLProgramDesc(const GrGLProgramDesc& desc) { *this = desc; }

    
    const uint32_t* asKey() const {
        SkASSERT(fInitialized);
        return reinterpret_cast<const uint32_t*>(fKey.get());
    }

    
    
    
    uint32_t keyLength() const { return *this->atOffset<uint32_t, kLengthOffset>(); }

    
    uint32_t getChecksum() const { return *this->atOffset<uint32_t, kChecksumOffset>(); }

    
    void setRandom(SkRandom*,
                   const GrGpuGL* gpu,
                   const GrRenderTarget* dummyDstRenderTarget,
                   const GrTexture* dummyDstCopyTexture,
                   const GrEffectStage* stages[],
                   int numColorStages,
                   int numCoverageStages,
                   int currAttribIndex);

    






    static void Build(const GrDrawState&,
                      bool isPoints,
                      GrDrawState::BlendOptFlags,
                      GrBlendCoeff srcCoeff,
                      GrBlendCoeff dstCoeff,
                      const GrGpuGL* gpu,
                      const GrDeviceCoordTexture* dstCopy,
                      SkTArray<const GrEffectStage*, true>* outColorStages,
                      SkTArray<const GrEffectStage*, true>* outCoverageStages,
                      GrGLProgramDesc* outDesc);

    int numColorEffects() const {
        SkASSERT(fInitialized);
        return this->getHeader().fColorEffectCnt;
    }

    int numCoverageEffects() const {
        SkASSERT(fInitialized);
        return this->getHeader().fCoverageEffectCnt;
    }

    int numTotalEffects() const { return this->numColorEffects() + this->numCoverageEffects(); }

    GrGLProgramDesc& operator= (const GrGLProgramDesc& other);

    bool operator== (const GrGLProgramDesc& other) const {
        SkASSERT(fInitialized && other.fInitialized);
        
        return 0 == memcmp(this->asKey(), other.asKey(), this->keyLength() & ~0x3);
    }

    bool operator!= (const GrGLProgramDesc& other) const {
        return !(*this == other);
    }

    static bool Less(const GrGLProgramDesc& a, const GrGLProgramDesc& b) {
        return memcmp(a.asKey(), b.asKey(), a.keyLength() & ~0x3) < 0;
    }

private:
    
    enum ColorInput {
        kSolidWhite_ColorInput,
        kTransBlack_ColorInput,
        kAttribute_ColorInput,
        kUniform_ColorInput,

        kColorInputCnt
    };

    enum CoverageOutput {
        
        kModulate_CoverageOutput,
        
        
        kSecondaryCoverage_CoverageOutput,
        
        
        kSecondaryCoverageISA_CoverageOutput,
        
        
        kSecondaryCoverageISC_CoverageOutput,
        
        
        kCombineWithDst_CoverageOutput,

        kCoverageOutputCnt
    };

    static bool CoverageOutputUsesSecondaryOutput(CoverageOutput co) {
        switch (co) {
            case kSecondaryCoverage_CoverageOutput: 
            case kSecondaryCoverageISA_CoverageOutput:
            case kSecondaryCoverageISC_CoverageOutput:
                return true;
            default:
                return false;
        }
    }

    struct KeyHeader {
        GrGLShaderBuilder::DstReadKey fDstReadKey;      
                                                        
                                                        
        GrGLShaderBuilder::FragPosKey fFragPosKey;      
                                                        
                                                        

        ColorInput                  fColorInput : 8;
        ColorInput                  fCoverageInput : 8;
        CoverageOutput              fCoverageOutput : 8;

        SkBool8                     fHasVertexCode;
        SkBool8                     fEmitsPointSize;

        
        
#if GR_GL_EXPERIMENTAL_GS
        SkBool8                     fExperimentalGS;
#endif

        int8_t                      fPositionAttributeIndex;
        int8_t                      fLocalCoordAttributeIndex;
        int8_t                      fColorAttributeIndex;
        int8_t                      fCoverageAttributeIndex;

        int8_t                      fColorEffectCnt;
        int8_t                      fCoverageEffectCnt;
    };

    
    
    enum {
        kLengthOffset = 0,
        kChecksumOffset = kLengthOffset + sizeof(uint32_t),
        kHeaderOffset = kChecksumOffset + sizeof(uint32_t),
        kHeaderSize = SkAlign4(sizeof(KeyHeader)),
        kEffectKeyOffset = kHeaderOffset + kHeaderSize,
    };

    template<typename T, size_t OFFSET> T* atOffset() {
        return reinterpret_cast<T*>(reinterpret_cast<intptr_t>(fKey.get()) + OFFSET);
    }

    template<typename T, size_t OFFSET> const T* atOffset() const {
        return reinterpret_cast<const T*>(reinterpret_cast<intptr_t>(fKey.get()) + OFFSET);
    }

    typedef GrGLEffect::EffectKey EffectKey;

    uint32_t* checksum() { return this->atOffset<uint32_t, kChecksumOffset>(); }
    KeyHeader* header() { return this->atOffset<KeyHeader, kHeaderOffset>(); }
    EffectKey* effectKeys() { return this->atOffset<EffectKey, kEffectKeyOffset>(); }

    const KeyHeader& getHeader() const { return *this->atOffset<KeyHeader, kHeaderOffset>(); }
    const EffectKey* getEffectKeys() const { return this->atOffset<EffectKey, kEffectKeyOffset>(); }

    static size_t KeyLength(int effectCnt) {
        GR_STATIC_ASSERT(!(sizeof(EffectKey) & 0x3));
        return kEffectKeyOffset + effectCnt * sizeof(EffectKey);
    }

    enum {
        kMaxPreallocEffects = 16,
        kPreAllocSize = kEffectKeyOffset +  kMaxPreallocEffects * sizeof(EffectKey),
    };

    SkAutoSMalloc<kPreAllocSize> fKey;
    bool fInitialized;

    
    
    friend class GrGLProgram;
    friend class GrGLShaderBuilder;
    friend class GrGLFullShaderBuilder;
    friend class GrGLFragmentOnlyShaderBuilder;
};

#endif
