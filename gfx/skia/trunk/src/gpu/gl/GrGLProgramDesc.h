






#ifndef GrGLProgramDesc_DEFINED
#define GrGLProgramDesc_DEFINED

#include "GrGLEffect.h"
#include "GrDrawState.h"
#include "GrGpu.h"

class GrGpuGL;

#ifdef SK_DEBUG
  
  
  #define GR_GL_EXPERIMENTAL_GS 1
#else
  #define GR_GL_EXPERIMENTAL_GS 0
#endif




class GrGLProgramDesc {
public:
    GrGLProgramDesc() {}
    GrGLProgramDesc(const GrGLProgramDesc& desc) { *this = desc; }

    
    const uint32_t* asKey() const {
        return reinterpret_cast<const uint32_t*>(fKey.begin());
    }

    
    
    
    uint32_t keyLength() const { return *this->atOffset<uint32_t, kLengthOffset>(); }

    
    uint32_t getChecksum() const { return *this->atOffset<uint32_t, kChecksumOffset>(); }

    
    bool setRandom(SkRandom*,
                   const GrGpuGL* gpu,
                   const GrRenderTarget* dummyDstRenderTarget,
                   const GrTexture* dummyDstCopyTexture,
                   const GrEffectStage* stages[],
                   int numColorStages,
                   int numCoverageStages,
                   int currAttribIndex);

    






    static bool Build(const GrDrawState&,
                      GrGpu::DrawType drawType,
                      GrDrawState::BlendOptFlags,
                      GrBlendCoeff srcCoeff,
                      GrBlendCoeff dstCoeff,
                      const GrGpuGL* gpu,
                      const GrDeviceCoordTexture* dstCopy,
                      SkTArray<const GrEffectStage*, true>* outColorStages,
                      SkTArray<const GrEffectStage*, true>* outCoverageStages,
                      GrGLProgramDesc* outDesc);

    int numColorEffects() const {
        return this->getHeader().fColorEffectCnt;
    }

    int numCoverageEffects() const {
        return this->getHeader().fCoverageEffectCnt;
    }

    int numTotalEffects() const { return this->numColorEffects() + this->numCoverageEffects(); }

    GrGLProgramDesc& operator= (const GrGLProgramDesc& other);

    bool operator== (const GrGLProgramDesc& other) const {
        
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
        uint8_t                     fDstReadKey;        
                                                        
                                                        
        uint8_t                     fFragPosKey;        
                                                        
                                                        
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
        
        
        kEffectKeyOffsetsAndLengthOffset = kHeaderOffset + kHeaderSize,
    };

    template<typename T, size_t OFFSET> T* atOffset() {
        return reinterpret_cast<T*>(reinterpret_cast<intptr_t>(fKey.begin()) + OFFSET);
    }

    template<typename T, size_t OFFSET> const T* atOffset() const {
        return reinterpret_cast<const T*>(reinterpret_cast<intptr_t>(fKey.begin()) + OFFSET);
    }

    KeyHeader* header() { return this->atOffset<KeyHeader, kHeaderOffset>(); }

    
    static bool GetEffectKeyAndUpdateStats(const GrEffectStage& stage,
                                           const GrGLCaps& caps,
                                           bool useExplicitLocalCoords,
                                           GrEffectKeyBuilder* b,
                                           uint16_t* effectKeySize,
                                           bool* setTrueIfReadsDst,
                                           bool* setTrueIfReadsPos,
                                           bool* setTrueIfHasVertexCode);

    void finalize();

    const KeyHeader& getHeader() const { return *this->atOffset<KeyHeader, kHeaderOffset>(); }

    
    class EffectKeyProvider {
    public:
        enum EffectType {
            kColor_EffectType,
            kCoverage_EffectType,
        };

        EffectKeyProvider(const GrGLProgramDesc* desc, EffectType type) : fDesc(desc) {
            
            fBaseIndex = kColor_EffectType == type ? 0 : desc->numColorEffects();
        }

        GrEffectKey get(int index) const {
            const uint16_t* offsetsAndLengths = reinterpret_cast<const uint16_t*>(
                fDesc->fKey.begin() + kEffectKeyOffsetsAndLengthOffset);
            
            
            uint16_t offset = offsetsAndLengths[2 * (fBaseIndex + index) + 0];
            uint16_t length = offsetsAndLengths[2 * (fBaseIndex + index) + 1];
            
            SkASSERT(0 == (length % sizeof(uint32_t)));
            return GrEffectKey(reinterpret_cast<const uint32_t*>(fDesc->fKey.begin() + offset),
                               length / sizeof(uint32_t));
        }
    private:
        const GrGLProgramDesc*  fDesc;
        int                     fBaseIndex;
    };

    enum {
        kMaxPreallocEffects = 8,
        kIntsPerEffect      = 4,    
        kPreAllocSize = kEffectKeyOffsetsAndLengthOffset +
                        kMaxPreallocEffects * sizeof(uint32_t) * kIntsPerEffect,
    };

    SkSTArray<kPreAllocSize, uint8_t, true> fKey;

    
    
    
    friend class GrGLProgram;
    friend class GrGLShaderBuilder;
    friend class GrGLFullShaderBuilder;
    friend class GrGLFragmentOnlyShaderBuilder;
};

#endif
