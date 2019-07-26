







#ifndef GrGLCaps_DEFINED
#define GrGLCaps_DEFINED

#include "GrDrawTargetCaps.h"
#include "GrGLStencilBuffer.h"
#include "SkTArray.h"
#include "SkTDArray.h"


#undef interface

class GrGLContextInfo;






class GrGLCaps : public GrDrawTargetCaps {
public:
    SK_DECLARE_INST_COUNT(GrGLCaps)

    typedef GrGLStencilBuffer::Format StencilFormat;

    



    enum MSFBOType {
        


        kNone_MSFBOType = 0,
        


        kDesktop_ARB_MSFBOType,
        


        kDesktop_EXT_MSFBOType,
        


        kES_3_0_MSFBOType,
        


        kES_Apple_MSFBOType,
        





        kES_IMG_MsToTexture_MSFBOType,
        



        kES_EXT_MsToTexture_MSFBOType,

        kLast_MSFBOType = kES_EXT_MsToTexture_MSFBOType
    };

    enum FBFetchType {
        kNone_FBFetchType,
        
        kEXT_FBFetchType,
        
        kNV_FBFetchType,

        kLast_FBFetchType = kNV_FBFetchType,
    };

    



    GrGLCaps();

    GrGLCaps(const GrGLCaps& caps);

    GrGLCaps& operator = (const GrGLCaps& caps);

    


    virtual void reset() SK_OVERRIDE;

    



    void init(const GrGLContextInfo& ctxInfo, const GrGLInterface* interface);

    




    void markConfigAsValidColorAttachment(GrPixelConfig config) {
        fVerifiedColorConfigs.markVerified(config);
    }

    



    bool isConfigVerifiedColorAttachment(GrPixelConfig config) const {
        return fVerifiedColorConfigs.isVerified(config);
    }

    





    void markColorConfigAndStencilFormatAsVerified(
                    GrPixelConfig config,
                    const GrGLStencilBuffer::Format& format);

    



    bool isColorConfigAndStencilFormatVerified(
                    GrPixelConfig config,
                    const GrGLStencilBuffer::Format& format) const;

    


    MSFBOType msFBOType() const { return fMSFBOType; }

    


    bool usesMSAARenderBuffers() const {
        return kNone_MSFBOType != fMSFBOType &&
               kES_IMG_MsToTexture_MSFBOType != fMSFBOType &&
               kES_EXT_MsToTexture_MSFBOType != fMSFBOType;
    }

    



    bool usesImplicitMSAAResolve() const {
        return kES_IMG_MsToTexture_MSFBOType == fMSFBOType ||
               kES_EXT_MsToTexture_MSFBOType == fMSFBOType;
    }

    FBFetchType fbFetchType() const { return fFBFetchType; }

    


    virtual SkString dump() const SK_OVERRIDE;

    




    const SkTArray<StencilFormat, true>& stencilFormats() const {
        return fStencilFormats;
    }

    
    int maxFragmentUniformVectors() const { return fMaxFragmentUniformVectors; }

    
    int maxVertexAttributes() const { return fMaxVertexAttributes; }

    
    int maxFragmentTextureUnits() const { return fMaxFragmentTextureUnits; }

    
    int maxFixedFunctionTextureCoords() const { return fMaxFixedFunctionTextureCoords; }

    
    bool rgba8RenderbufferSupport() const { return fRGBA8RenderbufferSupport; }

    
    bool bgraFormatSupport() const { return fBGRAFormatSupport; }

    




    bool bgraIsInternalFormat() const { return fBGRAIsInternalFormat; }

    
    bool textureSwizzleSupport() const { return fTextureSwizzleSupport; }

    
    bool unpackRowLengthSupport() const { return fUnpackRowLengthSupport; }

    
    bool unpackFlipYSupport() const { return fUnpackFlipYSupport; }

    
    bool packRowLengthSupport() const { return fPackRowLengthSupport; }

    
    bool packFlipYSupport() const { return fPackFlipYSupport; }

    
    bool textureUsageSupport() const { return fTextureUsageSupport; }

    
    bool texStorageSupport() const { return fTexStorageSupport; }

    
    bool textureRedSupport() const { return fTextureRedSupport; }

    
    bool imagingSupport() const { return fImagingSupport; }

    
    bool fragCoordConventionsSupport() const { return fFragCoordsConventionSupport; }

    
    bool vertexArrayObjectSupport() const { return fVertexArrayObjectSupport; }

    
    bool useNonVBOVertexAndIndexDynamicData() const {
        return fUseNonVBOVertexAndIndexDynamicData;
    }

    
    bool readPixelsSupported(const GrGLInterface* intf,
                             GrGLenum format,
                             GrGLenum type) const;

    bool isCoreProfile() const { return fIsCoreProfile; }

    bool fixedFunctionSupport() const { return fFixedFunctionSupport; }

    
    bool discardFBSupport() const { return fDiscardFBSupport; }

    bool fullClearIsFree() const { return fFullClearIsFree; }

private:
    



    struct VerifiedColorConfigs {
        VerifiedColorConfigs() {
            this->reset();
        }

        void reset() {
            for (int i = 0; i < kNumUints; ++i) {
                fVerifiedColorConfigs[i] = 0;
            }
        }

        static const int kNumUints = (kGrPixelConfigCnt  + 31) / 32;
        uint32_t fVerifiedColorConfigs[kNumUints];

        void markVerified(GrPixelConfig config) {
#if !GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT
                return;
#endif
            int u32Idx = config / 32;
            int bitIdx = config % 32;
            fVerifiedColorConfigs[u32Idx] |= 1 << bitIdx;
        }

        bool isVerified(GrPixelConfig config) const {
#if !GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT
            return false;
#endif
            int u32Idx = config / 32;
            int bitIdx = config % 32;
            return SkToBool(fVerifiedColorConfigs[u32Idx] & (1 << bitIdx));
        }
    };

    void initFSAASupport(const GrGLContextInfo&, const GrGLInterface*);
    void initStencilFormats(const GrGLContextInfo&);
    
    void initConfigRenderableTable(const GrGLContextInfo&);

    
    
    VerifiedColorConfigs fVerifiedColorConfigs;

    SkTArray<StencilFormat, true> fStencilFormats;
    
    
    
    SkTArray<VerifiedColorConfigs, true> fStencilVerifiedColorConfigs;

    int fMaxFragmentUniformVectors;
    int fMaxVertexAttributes;
    int fMaxFragmentTextureUnits;
    int fMaxFixedFunctionTextureCoords;

    MSFBOType fMSFBOType;

    FBFetchType fFBFetchType;

    bool fRGBA8RenderbufferSupport : 1;
    bool fBGRAFormatSupport : 1;
    bool fBGRAIsInternalFormat : 1;
    bool fTextureSwizzleSupport : 1;
    bool fUnpackRowLengthSupport : 1;
    bool fUnpackFlipYSupport : 1;
    bool fPackRowLengthSupport : 1;
    bool fPackFlipYSupport : 1;
    bool fTextureUsageSupport : 1;
    bool fTexStorageSupport : 1;
    bool fTextureRedSupport : 1;
    bool fImagingSupport  : 1;
    bool fTwoFormatLimit : 1;
    bool fFragCoordsConventionSupport : 1;
    bool fVertexArrayObjectSupport : 1;
    bool fUseNonVBOVertexAndIndexDynamicData : 1;
    bool fIsCoreProfile : 1;
    bool fFixedFunctionSupport : 1;
    bool fDiscardFBSupport : 1;
    bool fFullClearIsFree : 1;

    typedef GrDrawTargetCaps INHERITED;
};

#endif
