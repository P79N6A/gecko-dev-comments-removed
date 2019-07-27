



#include "WebGLContext.h"
#include "WebGLTexelConversions.h"

namespace mozilla {

using namespace WebGLTexelConversions;

namespace {













class WebGLImageConverter
{
    const size_t mWidth, mHeight;
    const void* const mSrcStart;
    void* const mDstStart;
    const ptrdiff_t mSrcStride, mDstStride;
    bool mAlreadyRun;
    bool mSuccess;

    




    template<MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) Format>
    static size_t NumElementsPerTexelForFormat() {
        switch (Format) {
            case WebGLTexelFormat::R8:
            case WebGLTexelFormat::A8:
            case WebGLTexelFormat::R16F:
            case WebGLTexelFormat::A16F:
            case WebGLTexelFormat::R32F:
            case WebGLTexelFormat::A32F:
            case WebGLTexelFormat::RGBA5551:
            case WebGLTexelFormat::RGBA4444:
            case WebGLTexelFormat::RGB565:
                return 1;
            case WebGLTexelFormat::RA8:
            case WebGLTexelFormat::RA16F:
            case WebGLTexelFormat::RA32F:
                return 2;
            case WebGLTexelFormat::RGB8:
            case WebGLTexelFormat::RGB16F:
            case WebGLTexelFormat::RGB32F:
                return 3;
            case WebGLTexelFormat::RGBA8:
            case WebGLTexelFormat::BGRA8:
            case WebGLTexelFormat::BGRX8:
            case WebGLTexelFormat::RGBA16F:
            case WebGLTexelFormat::RGBA32F:
                return 4;
            default:
                MOZ_ASSERT(false, "Unknown texel format. Coding mistake?");
                return 0;
        }
    }

    







    template<MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) SrcFormat,
             MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) DstFormat,
             MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelPremultiplicationOp) PremultiplicationOp>
    void run()
    {
        
        
        
        
        
        

        if (SrcFormat == DstFormat &&
            PremultiplicationOp == WebGLTexelPremultiplicationOp::None)
        {
            
            
            return;
        }

        
        
        
        
        const bool CanSrcFormatComeFromDOMElementOrImageData
            = SrcFormat == WebGLTexelFormat::BGRA8 ||
              SrcFormat == WebGLTexelFormat::BGRX8 ||
              SrcFormat == WebGLTexelFormat::A8 ||
              SrcFormat == WebGLTexelFormat::RGB565 ||
              SrcFormat == WebGLTexelFormat::RGBA8;
        if (!CanSrcFormatComeFromDOMElementOrImageData &&
            SrcFormat != DstFormat)
        {
            return;
        }

        
        if (!CanSrcFormatComeFromDOMElementOrImageData &&
            PremultiplicationOp == WebGLTexelPremultiplicationOp::Unpremultiply)
        {
            return;
        }

        
        
        
        
        
        if (!HasAlpha(SrcFormat) ||
            !HasColor(SrcFormat) ||
            !HasColor(DstFormat))
        {

            if (PremultiplicationOp != WebGLTexelPremultiplicationOp::None)
            {
                return;
            }
        }

        

        MOZ_ASSERT(!mAlreadyRun, "converter should be run only once!");
        mAlreadyRun = true;

        

        typedef
            typename DataTypeForFormat<SrcFormat>::Type
            SrcType;
        typedef
            typename DataTypeForFormat<DstFormat>::Type
            DstType;

        const MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) IntermediateSrcFormat
            = IntermediateFormat<SrcFormat>::Value;
        const MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) IntermediateDstFormat
            = IntermediateFormat<DstFormat>::Value;
        typedef
            typename DataTypeForFormat<IntermediateSrcFormat>::Type
            IntermediateSrcType;
        typedef
            typename DataTypeForFormat<IntermediateDstFormat>::Type
            IntermediateDstType;

        const size_t NumElementsPerSrcTexel = NumElementsPerTexelForFormat<SrcFormat>();
        const size_t NumElementsPerDstTexel = NumElementsPerTexelForFormat<DstFormat>();
        const size_t MaxElementsPerTexel = 4;
        MOZ_ASSERT(NumElementsPerSrcTexel <= MaxElementsPerTexel, "unhandled format");
        MOZ_ASSERT(NumElementsPerDstTexel <= MaxElementsPerTexel, "unhandled format");

        
        
        
        
        
        
        MOZ_ASSERT(mSrcStride % sizeof(SrcType) == 0 &&
                   mDstStride % sizeof(DstType) == 0,
                   "Unsupported: texture stride is not a multiple of sizeof(type)");
        const ptrdiff_t srcStrideInElements = mSrcStride / sizeof(SrcType);
        const ptrdiff_t dstStrideInElements = mDstStride / sizeof(DstType);

        const SrcType *srcRowStart = static_cast<const SrcType*>(mSrcStart);
        DstType *dstRowStart = static_cast<DstType*>(mDstStart);

        
        for (size_t i = 0; i < mHeight; ++i) {
            const SrcType *srcRowEnd = srcRowStart + mWidth * NumElementsPerSrcTexel;
            const SrcType *srcPtr = srcRowStart;
            DstType *dstPtr = dstRowStart;
            while (srcPtr != srcRowEnd) {
                
                
                
                
                
                IntermediateSrcType unpackedSrc[MaxElementsPerTexel];
                IntermediateDstType unpackedDst[MaxElementsPerTexel];

                
                
                unpack<SrcFormat>(srcPtr, unpackedSrc);
                
                
                convertType(unpackedSrc, unpackedDst);
                
                
                pack<DstFormat, PremultiplicationOp>(unpackedDst, dstPtr);

                srcPtr += NumElementsPerSrcTexel;
                dstPtr += NumElementsPerDstTexel;
            }
            srcRowStart += srcStrideInElements;
            dstRowStart += dstStrideInElements;
        }

        mSuccess = true;
        return;
    }

    template<MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) SrcFormat,
             MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) DstFormat>
    void run(WebGLTexelPremultiplicationOp premultiplicationOp)
    {
        #define WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(PremultiplicationOp) \
            case PremultiplicationOp: \
                return run<SrcFormat, DstFormat, PremultiplicationOp>();

        switch (premultiplicationOp) {
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(WebGLTexelPremultiplicationOp::None)
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(WebGLTexelPremultiplicationOp::Premultiply)
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(WebGLTexelPremultiplicationOp::Unpremultiply)
            default:
                MOZ_ASSERT(false, "unhandled case. Coding mistake?");
        }

        #undef WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP
    }

    template<MOZ_ENUM_CLASS_ENUM_TYPE(WebGLTexelFormat) SrcFormat>
    void run(WebGLTexelFormat dstFormat,
             WebGLTexelPremultiplicationOp premultiplicationOp)
    {
        #define WEBGLIMAGECONVERTER_CASE_DSTFORMAT(DstFormat) \
            case DstFormat: \
                return run<SrcFormat, DstFormat>(premultiplicationOp);

        switch (dstFormat) {
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::R8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::A8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::R16F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::A16F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::R32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::A32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RA8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RA16F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RA32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGB8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGB565)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGB16F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGB32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGBA8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGBA5551)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGBA4444)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGBA16F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(WebGLTexelFormat::RGBA32F)
            default:
                MOZ_ASSERT(false, "unhandled case. Coding mistake?");
        }

        #undef WEBGLIMAGECONVERTER_CASE_DSTFORMAT
    }

public:

    void run(WebGLTexelFormat srcFormat,
             WebGLTexelFormat dstFormat,
             WebGLTexelPremultiplicationOp premultiplicationOp)
    {
        #define WEBGLIMAGECONVERTER_CASE_SRCFORMAT(SrcFormat) \
            case SrcFormat: \
                return run<SrcFormat>(dstFormat, premultiplicationOp);

        switch (srcFormat) {
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::R8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::A8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::R16F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::A16F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::R32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::A32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RA16F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RA32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGB8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::BGRX8) 
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGB565)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGB16F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGB32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGBA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::BGRA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGBA5551)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGBA4444)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGBA16F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(WebGLTexelFormat::RGBA32F)
            default:
                MOZ_ASSERT(false, "unhandled case. Coding mistake?");
        }

        #undef WEBGLIMAGECONVERTER_CASE_SRCFORMAT
    }

    WebGLImageConverter(size_t width, size_t height,
                        const void* srcStart, void* dstStart,
                        ptrdiff_t srcStride, ptrdiff_t dstStride)
        : mWidth(width), mHeight(height),
          mSrcStart(srcStart), mDstStart(dstStart),
          mSrcStride(srcStride), mDstStride(dstStride),
          mAlreadyRun(false), mSuccess(false)
    {}

    bool Success() const {
        return mSuccess;
    }
};

} 

void
WebGLContext::ConvertImage(size_t width, size_t height, size_t srcStride, size_t dstStride,
                           const uint8_t* src, uint8_t *dst,
                           WebGLTexelFormat srcFormat, bool srcPremultiplied,
                           WebGLTexelFormat dstFormat, bool dstPremultiplied,
                           size_t dstTexelSize)
{
    if (width <= 0 || height <= 0)
        return;

    const bool FormatsRequireNoPremultiplicationOp =
        !HasAlpha(srcFormat) ||
        !HasColor(srcFormat) ||
        !HasColor(dstFormat);

    if (srcFormat == dstFormat &&
        (FormatsRequireNoPremultiplicationOp || srcPremultiplied == dstPremultiplied))
    {
        
        
        
        
        
        
        

        MOZ_ASSERT(mPixelStoreFlipY || srcStride != dstStride, "Performance trap -- should handle this case earlier, to avoid memcpy");

        size_t row_size = width * dstTexelSize; 
        const uint8_t* ptr = src;
        const uint8_t* src_end = src + height * srcStride;

        uint8_t* dst_row = mPixelStoreFlipY
                           ? dst + (height-1) * dstStride
                           : dst;
        ptrdiff_t dstStrideSigned(dstStride);
        ptrdiff_t dst_delta = mPixelStoreFlipY ? -dstStrideSigned : dstStrideSigned;

        while(ptr != src_end) {
            memcpy(dst_row, ptr, row_size);
            ptr += srcStride;
            dst_row += dst_delta;
        }
        return;
    }

    uint8_t* dstStart = dst;
    ptrdiff_t signedDstStride = dstStride;
    if (mPixelStoreFlipY) {
        dstStart = dst + (height - 1) * dstStride;
        signedDstStride = -signedDstStride;
    }

    WebGLImageConverter converter(width, height, src, dstStart, srcStride, signedDstStride);

    const WebGLTexelPremultiplicationOp premultiplicationOp
        = FormatsRequireNoPremultiplicationOp     ? WebGLTexelPremultiplicationOp::None
        : (!srcPremultiplied && dstPremultiplied) ? WebGLTexelPremultiplicationOp::Premultiply
        : (srcPremultiplied && !dstPremultiplied) ? WebGLTexelPremultiplicationOp::Unpremultiply
                                                  : WebGLTexelPremultiplicationOp::None;

    converter.run(srcFormat, dstFormat, premultiplicationOp);

    if (!converter.Success()) {
        
        
        
        NS_RUNTIMEABORT("programming mistake in WebGL texture conversions");
    }
}

} 
