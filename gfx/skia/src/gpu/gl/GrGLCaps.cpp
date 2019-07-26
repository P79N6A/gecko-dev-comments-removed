







#include "GrGLCaps.h"
#include "GrGLContext.h"
#include "SkTSearch.h"

SK_DEFINE_INST_COUNT(GrGLCaps)

GrGLCaps::GrGLCaps() {
    this->reset();
}

void GrGLCaps::reset() {
    INHERITED::reset();

    fVerifiedColorConfigs.reset();
    fStencilFormats.reset();
    fStencilVerifiedColorConfigs.reset();
    fMSFBOType = kNone_MSFBOType;
    fCoverageAAType = kNone_CoverageAAType;
    fMaxFragmentUniformVectors = 0;
    fMaxVertexAttributes = 0;
    fRGBA8RenderbufferSupport = false;
    fBGRAFormatSupport = false;
    fBGRAIsInternalFormat = false;
    fTextureSwizzleSupport = false;
    fUnpackRowLengthSupport = false;
    fUnpackFlipYSupport = false;
    fPackRowLengthSupport = false;
    fPackFlipYSupport = false;
    fTextureUsageSupport = false;
    fTexStorageSupport = false;
    fTextureRedSupport = false;
    fImagingSupport = false;
    fTwoFormatLimit = false;
    fFragCoordsConventionSupport = false;
    fVertexArrayObjectSupport = false;
    fUseNonVBOVertexAndIndexDynamicData = false;
    fIsCoreProfile = false;
}

GrGLCaps::GrGLCaps(const GrGLCaps& caps) : GrDrawTargetCaps() {
    *this = caps;
}

GrGLCaps& GrGLCaps::operator = (const GrGLCaps& caps) {
    INHERITED::operator=(caps);
    fVerifiedColorConfigs = caps.fVerifiedColorConfigs;
    fStencilFormats = caps.fStencilFormats;
    fStencilVerifiedColorConfigs = caps.fStencilVerifiedColorConfigs;
    fMaxFragmentUniformVectors = caps.fMaxFragmentUniformVectors;
    fMaxVertexAttributes = caps.fMaxVertexAttributes;
    fMSFBOType = caps.fMSFBOType;
    fCoverageAAType = caps.fCoverageAAType;
    fMSAACoverageModes = caps.fMSAACoverageModes;
    fRGBA8RenderbufferSupport = caps.fRGBA8RenderbufferSupport;
    fBGRAFormatSupport = caps.fBGRAFormatSupport;
    fBGRAIsInternalFormat = caps.fBGRAIsInternalFormat;
    fTextureSwizzleSupport = caps.fTextureSwizzleSupport;
    fUnpackRowLengthSupport = caps.fUnpackRowLengthSupport;
    fUnpackFlipYSupport = caps.fUnpackFlipYSupport;
    fPackRowLengthSupport = caps.fPackRowLengthSupport;
    fPackFlipYSupport = caps.fPackFlipYSupport;
    fTextureUsageSupport = caps.fTextureUsageSupport;
    fTexStorageSupport = caps.fTexStorageSupport;
    fTextureRedSupport = caps.fTextureRedSupport;
    fImagingSupport = caps.fImagingSupport;
    fTwoFormatLimit = caps.fTwoFormatLimit;
    fFragCoordsConventionSupport = caps.fFragCoordsConventionSupport;
    fVertexArrayObjectSupport = caps.fVertexArrayObjectSupport;
    fUseNonVBOVertexAndIndexDynamicData = caps.fUseNonVBOVertexAndIndexDynamicData;
    fIsCoreProfile = caps.fIsCoreProfile;

    return *this;
}

void GrGLCaps::init(const GrGLContextInfo& ctxInfo, const GrGLInterface* gli) {

    this->reset();
    if (!ctxInfo.isInitialized()) {
        return;
    }

    GrGLBinding binding = ctxInfo.binding();
    GrGLVersion version = ctxInfo.version();

    



    if (kES2_GrGLBinding == binding) {
        GR_GL_GetIntegerv(gli, GR_GL_MAX_FRAGMENT_UNIFORM_VECTORS,
                          &fMaxFragmentUniformVectors);
    } else {
        GrAssert(kDesktop_GrGLBinding == binding);
        GrGLint max;
        GR_GL_GetIntegerv(gli, GR_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max);
        fMaxFragmentUniformVectors = max / 4;
    }
    GR_GL_GetIntegerv(gli, GR_GL_MAX_VERTEX_ATTRIBS, &fMaxVertexAttributes);

    if (kDesktop_GrGLBinding == binding) {
        fRGBA8RenderbufferSupport = true;
    } else {
        fRGBA8RenderbufferSupport = ctxInfo.hasExtension("GL_OES_rgb8_rgba8") ||
                                    ctxInfo.hasExtension("GL_ARM_rgba8");
    }

    if (kDesktop_GrGLBinding == binding) {
        fBGRAFormatSupport = version >= GR_GL_VER(1,2) ||
                             ctxInfo.hasExtension("GL_EXT_bgra");
    } else {
        if (ctxInfo.hasExtension("GL_APPLE_texture_format_BGRA8888")) {
            fBGRAFormatSupport = true;
        } else if (ctxInfo.hasExtension("GL_EXT_texture_format_BGRA8888")) {
            fBGRAFormatSupport = true;
            fBGRAIsInternalFormat = true;
        }
        GrAssert(fBGRAFormatSupport ||
                 kSkia8888_GrPixelConfig != kBGRA_8888_GrPixelConfig);
    }

    if (kDesktop_GrGLBinding == binding) {
        fTextureSwizzleSupport = version >= GR_GL_VER(3,3) ||
                                 ctxInfo.hasExtension("GL_ARB_texture_swizzle");
    } else {
        fTextureSwizzleSupport = false;
    }

    if (kDesktop_GrGLBinding == binding) {
        fUnpackRowLengthSupport = true;
        fUnpackFlipYSupport = false;
        fPackRowLengthSupport = true;
        fPackFlipYSupport = false;
    } else {
        fUnpackRowLengthSupport =ctxInfo.hasExtension("GL_EXT_unpack_subimage");
        fUnpackFlipYSupport = ctxInfo.hasExtension("GL_CHROMIUM_flipy");
        
        fPackRowLengthSupport = false;
        fPackFlipYSupport =
            ctxInfo.hasExtension("GL_ANGLE_pack_reverse_row_order");
    }

    fTextureUsageSupport = (kES2_GrGLBinding == binding) &&
                            ctxInfo.hasExtension("GL_ANGLE_texture_usage");

    
    fTexStorageSupport = (kDesktop_GrGLBinding == binding &&
                          version >= GR_GL_VER(4,2)) ||
                         ctxInfo.hasExtension("GL_ARB_texture_storage") ||
                         ctxInfo.hasExtension("GL_EXT_texture_storage");

    
    if (kDesktop_GrGLBinding == binding) {
        fTextureRedSupport = version >= GR_GL_VER(3,0) ||
                             ctxInfo.hasExtension("GL_ARB_texture_rg");
    } else {
        fTextureRedSupport = ctxInfo.hasExtension("GL_EXT_texture_rg");
    }

    fImagingSupport = kDesktop_GrGLBinding == binding &&
                      ctxInfo.hasExtension("GL_ARB_imaging");

    
    
    
    fTwoFormatLimit = kES2_GrGLBinding == binding;

    
    
    if (kIntel_GrGLVendor != ctxInfo.vendor()) {
        fFragCoordsConventionSupport = ctxInfo.glslGeneration() >= k150_GrGLSLGeneration ||
                                       ctxInfo.hasExtension("GL_ARB_fragment_coord_conventions");
    }

    
    
    
    
    if (!GR_GL_MUST_USE_VBO &&
        (kARM_GrGLVendor == ctxInfo.vendor() || kImagination_GrGLVendor == ctxInfo.vendor())) {
        fUseNonVBOVertexAndIndexDynamicData = true;
    }

    if (kDesktop_GrGLBinding == binding && version >= GR_GL_VER(3, 2)) {
        GrGLint profileMask;
        GR_GL_GetIntegerv(gli, GR_GL_CONTEXT_PROFILE_MASK, &profileMask);
        fIsCoreProfile = SkToBool(profileMask & GR_GL_CONTEXT_CORE_PROFILE_BIT);
    }

    if (kDesktop_GrGLBinding == binding) {
        fVertexArrayObjectSupport = version >= GR_GL_VER(3, 0) ||
                                    ctxInfo.hasExtension("GL_ARB_vertex_array_object");
    } else {
        fVertexArrayObjectSupport = ctxInfo.hasExtension("GL_OES_vertex_array_object");
    }

    this->initFSAASupport(ctxInfo, gli);
    this->initStencilFormats(ctxInfo);

    


    GrGLint maxTextureUnits;
    
    
    
    GR_GL_GetIntegerv(gli, GR_GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

    GrGLint numFormats;
    GR_GL_GetIntegerv(gli, GR_GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numFormats);
    SkAutoSTMalloc<10, GrGLint> formats(numFormats);
    GR_GL_GetIntegerv(gli, GR_GL_COMPRESSED_TEXTURE_FORMATS, formats);
    for (int i = 0; i < numFormats; ++i) {
        if (formats[i] == GR_GL_PALETTE8_RGBA8) {
            f8BitPaletteSupport = true;
            break;
        }
    }

    if (kDesktop_GrGLBinding == binding) {
        
        
        
        fTwoSidedStencilSupport = (ctxInfo.version() >= GR_GL_VER(2,0));
        
        fStencilWrapOpsSupport = (ctxInfo.version() >= GR_GL_VER(1,4)) ||
                                  ctxInfo.hasExtension("GL_EXT_stencil_wrap");
    } else {
        
        fTwoSidedStencilSupport = true;
        fStencilWrapOpsSupport = true;
    }

    if (kDesktop_GrGLBinding == binding) {
        fBufferLockSupport = true; 
                                   
    } else {
        fBufferLockSupport = ctxInfo.hasExtension("GL_OES_mapbuffer");
    }

    if (kDesktop_GrGLBinding == binding) {
        if (ctxInfo.version() >= GR_GL_VER(2,0) ||
            ctxInfo.hasExtension("GL_ARB_texture_non_power_of_two")) {
            fNPOTTextureTileSupport = true;
        } else {
            fNPOTTextureTileSupport = false;
        }
    } else {
        
        fNPOTTextureTileSupport = ctxInfo.hasExtension("GL_OES_texture_npot");
    }

    fHWAALineSupport = (kDesktop_GrGLBinding == binding);

    GR_GL_GetIntegerv(gli, GR_GL_MAX_TEXTURE_SIZE, &fMaxTextureSize);
    GR_GL_GetIntegerv(gli, GR_GL_MAX_RENDERBUFFER_SIZE, &fMaxRenderTargetSize);
    
    
    fMaxRenderTargetSize = GrMin(fMaxTextureSize, fMaxRenderTargetSize);

    fPathStencilingSupport = GR_GL_USE_NV_PATH_RENDERING &&
                             ctxInfo.hasExtension("GL_NV_path_rendering");

    
    if (kDesktop_GrGLBinding == binding) {
        fDualSourceBlendingSupport = ctxInfo.version() >= GR_GL_VER(3,3) ||
                                     ctxInfo.hasExtension("GL_ARB_blend_func_extended");
        fShaderDerivativeSupport = true;
        
        fGeometryShaderSupport = ctxInfo.version() >= GR_GL_VER(3,2) &&
                                 ctxInfo.glslGeneration() >= k150_GrGLSLGeneration;
    } else {
        fShaderDerivativeSupport = ctxInfo.hasExtension("GL_OES_standard_derivatives");
    }

    if (GrGLCaps::kImaginationES_MSFBOType == fMSFBOType) {
        GR_GL_GetIntegerv(gli, GR_GL_MAX_SAMPLES_IMG, &fMaxSampleCount);
    } else if (GrGLCaps::kNone_MSFBOType != fMSFBOType) {
        GR_GL_GetIntegerv(gli, GR_GL_MAX_SAMPLES, &fMaxSampleCount);
    }
}

bool GrGLCaps::readPixelsSupported(const GrGLInterface* intf,
                                   GrGLenum format,
                                   GrGLenum type) const {
    if (GR_GL_RGBA == format && GR_GL_UNSIGNED_BYTE == type) {
        
        return true;
    }

    if (!fTwoFormatLimit) {
        
        return true;
    }

    GrGLint otherFormat = GR_GL_RGBA;
    GrGLint otherType = GR_GL_UNSIGNED_BYTE;

    
    
    GR_GL_GetIntegerv(intf,
                      GR_GL_IMPLEMENTATION_COLOR_READ_FORMAT,
                      &otherFormat);

    GR_GL_GetIntegerv(intf,
                      GR_GL_IMPLEMENTATION_COLOR_READ_TYPE,
                      &otherType);

    return (GrGLenum)otherFormat == format && (GrGLenum)otherType == type;
}

namespace {
int coverage_mode_compare(const GrGLCaps::MSAACoverageMode* left,
                          const GrGLCaps::MSAACoverageMode* right) {
    if (left->fCoverageSampleCnt < right->fCoverageSampleCnt) {
        return -1;
    } else if (right->fCoverageSampleCnt < left->fCoverageSampleCnt) {
        return 1;
    } else if (left->fColorSampleCnt < right->fColorSampleCnt) {
        return -1;
    } else if (right->fColorSampleCnt < left->fColorSampleCnt) {
        return 1;
    }
    return 0;
}
}

void GrGLCaps::initFSAASupport(const GrGLContextInfo& ctxInfo, const GrGLInterface* gli) {

    fMSFBOType = kNone_MSFBOType;
    if (kDesktop_GrGLBinding != ctxInfo.binding()) {
       if (ctxInfo.hasExtension("GL_CHROMIUM_framebuffer_multisample")) {
           
           
           fMSFBOType = kDesktopEXT_MSFBOType;
       } else if (ctxInfo.hasExtension("GL_APPLE_framebuffer_multisample")) {
           fMSFBOType = kAppleES_MSFBOType;
       } else if (ctxInfo.hasExtension("GL_IMG_multisampled_render_to_texture")) {
           fMSFBOType = kImaginationES_MSFBOType;
       }
    } else {
        if ((ctxInfo.version() >= GR_GL_VER(3,0)) ||
            ctxInfo.hasExtension("GL_ARB_framebuffer_object")) {
            fMSFBOType = GrGLCaps::kDesktopARB_MSFBOType;
        } else if (ctxInfo.hasExtension("GL_EXT_framebuffer_multisample") &&
                   ctxInfo.hasExtension("GL_EXT_framebuffer_blit")) {
            fMSFBOType = GrGLCaps::kDesktopEXT_MSFBOType;
        }
        
        
        
        
        if (ctxInfo.hasExtension("GL_NV_framebuffer_multisample_coverage")) {
            fCoverageAAType = kNVDesktop_CoverageAAType;
            GrGLint count;
            GR_GL_GetIntegerv(gli,
                              GR_GL_MAX_MULTISAMPLE_COVERAGE_MODES,
                              &count);
            fMSAACoverageModes.setCount(count);
            GR_GL_GetIntegerv(gli,
                              GR_GL_MULTISAMPLE_COVERAGE_MODES,
                              (int*)&fMSAACoverageModes[0]);
            
            
            qsort(&fMSAACoverageModes[0],
                    count,
                    sizeof(MSAACoverageMode),
                    SkCastForQSort(coverage_mode_compare));
        }
    }
}

const GrGLCaps::MSAACoverageMode& GrGLCaps::getMSAACoverageMode(int desiredSampleCount) const {
    static const MSAACoverageMode kNoneMode = {0, 0};
    if (0 == fMSAACoverageModes.count()) {
        return kNoneMode;
    } else {
        GrAssert(kNone_CoverageAAType != fCoverageAAType);
        int max = (fMSAACoverageModes.end() - 1)->fCoverageSampleCnt;
        desiredSampleCount = GrMin(desiredSampleCount, max);
        MSAACoverageMode desiredMode = {desiredSampleCount, 0};
        int idx = SkTSearch<MSAACoverageMode>(&fMSAACoverageModes[0],
                                              fMSAACoverageModes.count(),
                                              desiredMode,
                                              sizeof(MSAACoverageMode),
                                              &coverage_mode_compare);
        if (idx < 0) {
            idx = ~idx;
        }
        GrAssert(idx >= 0 && idx < fMSAACoverageModes.count());
        return fMSAACoverageModes[idx];
    }
}

namespace {
const GrGLuint kUnknownBitCount = GrGLStencilBuffer::kUnknownBitCount;
}

void GrGLCaps::initStencilFormats(const GrGLContextInfo& ctxInfo) {

    
    

    
    

    static const StencilFormat
                  
        gS8    = {GR_GL_STENCIL_INDEX8,   8,                8,                false},
        gS16   = {GR_GL_STENCIL_INDEX16,  16,               16,               false},
        gD24S8 = {GR_GL_DEPTH24_STENCIL8, 8,                32,               true },
        gS4    = {GR_GL_STENCIL_INDEX4,   4,                4,                false},
    
        gDS    = {GR_GL_DEPTH_STENCIL,    kUnknownBitCount, kUnknownBitCount, true };

    if (kDesktop_GrGLBinding == ctxInfo.binding()) {
        bool supportsPackedDS =
            ctxInfo.version() >= GR_GL_VER(3,0) ||
            ctxInfo.hasExtension("GL_EXT_packed_depth_stencil") ||
            ctxInfo.hasExtension("GL_ARB_framebuffer_object");

        
        
        
        fStencilFormats.push_back() = gS8;
        fStencilFormats.push_back() = gS16;
        if (supportsPackedDS) {
            fStencilFormats.push_back() = gD24S8;
        }
        fStencilFormats.push_back() = gS4;
        if (supportsPackedDS) {
            fStencilFormats.push_back() = gDS;
        }
    } else {
        
        
        

        fStencilFormats.push_back() = gS8;
        
        if (ctxInfo.hasExtension("GL_OES_packed_depth_stencil")) {
            fStencilFormats.push_back() = gD24S8;
        }
        if (ctxInfo.hasExtension("GL_OES_stencil4")) {
            fStencilFormats.push_back() = gS4;
        }
    }
    GrAssert(0 == fStencilVerifiedColorConfigs.count());
    fStencilVerifiedColorConfigs.push_back_n(fStencilFormats.count());
}

void GrGLCaps::markColorConfigAndStencilFormatAsVerified(
                                    GrPixelConfig config,
                                    const GrGLStencilBuffer::Format& format) {
#if !GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT
    return;
#endif
    GrAssert((unsigned)config < (unsigned)kGrPixelConfigCnt);
    GrAssert(fStencilFormats.count() == fStencilVerifiedColorConfigs.count());
    int count = fStencilFormats.count();
    
    
    GrAssert(count < 16);
    for (int i = 0; i < count; ++i) {
        if (format.fInternalFormat ==
            fStencilFormats[i].fInternalFormat) {
            fStencilVerifiedColorConfigs[i].markVerified(config);
            return;
        }
    }
    GrCrash("Why are we seeing a stencil format that "
            "GrGLCaps doesn't know about.");
}

bool GrGLCaps::isColorConfigAndStencilFormatVerified(
                                GrPixelConfig config,
                                const GrGLStencilBuffer::Format& format) const {
#if !GR_GL_CHECK_FBO_STATUS_ONCE_PER_FORMAT
    return false;
#endif
    GrAssert((unsigned)config < (unsigned)kGrPixelConfigCnt);
    int count = fStencilFormats.count();
    
    
    GrAssert(count < 16);
    for (int i = 0; i < count; ++i) {
        if (format.fInternalFormat ==
            fStencilFormats[i].fInternalFormat) {
            return fStencilVerifiedColorConfigs[i].isVerified(config);
        }
    }
    GrCrash("Why are we seeing a stencil format that "
            "GLCaps doesn't know about.");
    return false;
}

void GrGLCaps::print() const {

    INHERITED::print();

    GrPrintf("--- GL-Specific ---\n");
    for (int i = 0; i < fStencilFormats.count(); ++i) {
        GrPrintf("Stencil Format %d, stencil bits: %02d, total bits: %02d\n",
                 i,
                 fStencilFormats[i].fStencilBits,
                 fStencilFormats[i].fTotalBits);
    }

    GR_STATIC_ASSERT(0 == kNone_MSFBOType);
    GR_STATIC_ASSERT(1 == kDesktopARB_MSFBOType);
    GR_STATIC_ASSERT(2 == kDesktopEXT_MSFBOType);
    GR_STATIC_ASSERT(3 == kAppleES_MSFBOType);
    GR_STATIC_ASSERT(4 == kImaginationES_MSFBOType);
    static const char* gMSFBOExtStr[] = {
        "None",
        "ARB",
        "EXT",
        "Apple",
        "IMG",
    };
    GrPrintf("MSAA Type: %s\n", gMSFBOExtStr[fMSFBOType]);
    GrPrintf("Max FS Uniform Vectors: %d\n", fMaxFragmentUniformVectors);
    GrPrintf("Max Vertex Attributes: %d\n", fMaxVertexAttributes);
    GrPrintf("Support RGBA8 Render Buffer: %s\n", (fRGBA8RenderbufferSupport ? "YES": "NO"));
    GrPrintf("BGRA support: %s\n", (fBGRAFormatSupport ? "YES": "NO"));
    GrPrintf("BGRA is an internal format: %s\n", (fBGRAIsInternalFormat ? "YES": "NO"));
    GrPrintf("Support texture swizzle: %s\n", (fTextureSwizzleSupport ? "YES": "NO"));
    GrPrintf("Unpack Row length support: %s\n", (fUnpackRowLengthSupport ? "YES": "NO"));
    GrPrintf("Unpack Flip Y support: %s\n", (fUnpackFlipYSupport ? "YES": "NO"));
    GrPrintf("Pack Row length support: %s\n", (fPackRowLengthSupport ? "YES": "NO"));
    GrPrintf("Pack Flip Y support: %s\n", (fPackFlipYSupport ? "YES": "NO"));

    GrPrintf("Texture Usage support: %s\n", (fTextureUsageSupport ? "YES": "NO"));
    GrPrintf("Texture Storage support: %s\n", (fTexStorageSupport ? "YES": "NO"));
    GrPrintf("GL_R support: %s\n", (fTextureRedSupport ? "YES": "NO"));
    GrPrintf("GL_ARB_imaging support: %s\n", (fImagingSupport ? "YES": "NO"));
    GrPrintf("Two Format Limit: %s\n", (fTwoFormatLimit ? "YES": "NO"));
    GrPrintf("Fragment coord conventions support: %s\n",
             (fFragCoordsConventionSupport ? "YES": "NO"));
    GrPrintf("Vertex array object support: %s\n", (fVertexArrayObjectSupport ? "YES": "NO"));
    GrPrintf("Use non-VBO for dynamic data: %s\n",
             (fUseNonVBOVertexAndIndexDynamicData ? "YES" : "NO"));
    GrPrintf("Core Profile: %s\n", (fIsCoreProfile ? "YES" : "NO"));
}
