



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

    




    template<int Format>
    static size_t NumElementsPerTexelForFormat() {
        switch (Format) {
            case R8:
            case A8:
            case R32F:
            case A32F:
            case RGBA5551:
            case RGBA4444:
            case RGB565:
                return 1;
            case RA8:
            case RA32F:
                return 2;
            case RGB8:
            case RGB32F:
                return 3;
            case RGBA8:
            case BGRA8:
            case BGRX8:
            case RGBA32F:
                return 4;
            default:
                MOZ_ASSERT(false, "Unknown texel format. Coding mistake?");
                return 0;
        }
    }

    







    template<WebGLTexelFormat SrcFormat,
             WebGLTexelFormat DstFormat,
             WebGLTexelPremultiplicationOp PremultiplicationOp>
    void run()
    {
        
        
        
        
        
        

        if (SrcFormat == DstFormat &&
            PremultiplicationOp == NoPremultiplicationOp)
        {
            
            
            return;
        }

        
        
        
        
        const bool CanSrcFormatComeFromDOMElementOrImageData
            = SrcFormat == BGRA8 ||
              SrcFormat == BGRX8 ||
              SrcFormat == A8 ||
              SrcFormat == RGB565 ||
              SrcFormat == RGBA8;
        if (!CanSrcFormatComeFromDOMElementOrImageData &&
            SrcFormat != DstFormat)
        {
            return;
        }

        
        if (!CanSrcFormatComeFromDOMElementOrImageData &&
            PremultiplicationOp == Unpremultiply)
        {
            return;
        }

        
        
        
        
        
        if (!HasAlpha(SrcFormat) ||
            !HasColor(SrcFormat) ||
            !HasColor(DstFormat))
        {

            if (PremultiplicationOp != NoPremultiplicationOp)
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

        const int IntermediateSrcFormat
            = IntermediateFormat<SrcFormat>::Value;
        const int IntermediateDstFormat
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

    template<WebGLTexelFormat SrcFormat, WebGLTexelFormat DstFormat>
    void run(WebGLTexelPremultiplicationOp premultiplicationOp)
    {
        #define WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(PremultiplicationOp) \
            case PremultiplicationOp: \
                return run<SrcFormat, DstFormat, PremultiplicationOp>();

        switch (premultiplicationOp) {
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(NoPremultiplicationOp)
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(Premultiply)
            WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP(Unpremultiply)
            default:
                MOZ_ASSERT(false, "unhandled case. Coding mistake?");
        }

        #undef WEBGLIMAGECONVERTER_CASE_PREMULTIPLICATIONOP
    }

    template<WebGLTexelFormat SrcFormat>
    void run(WebGLTexelFormat dstFormat,
             WebGLTexelPremultiplicationOp premultiplicationOp)
    {
        #define WEBGLIMAGECONVERTER_CASE_DSTFORMAT(DstFormat) \
            case DstFormat: \
                return run<SrcFormat, DstFormat>(premultiplicationOp);

        switch (dstFormat) {
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(R8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(A8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(R32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(A32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RA8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RA32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGB8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGB565)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGB32F)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGBA8)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGBA5551)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGBA4444)
            WEBGLIMAGECONVERTER_CASE_DSTFORMAT(RGBA32F)
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
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(R8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(A8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(R32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(A32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RA32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGB8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(BGRX8) 
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGB565)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGB32F)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGBA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(BGRA8)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGBA5551)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGBA4444)
            WEBGLIMAGECONVERTER_CASE_SRCFORMAT(RGBA32F)
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
        = FormatsRequireNoPremultiplicationOp     ? NoPremultiplicationOp
        : (!srcPremultiplied && dstPremultiplied) ? Premultiply
        : (srcPremultiplied && !dstPremultiplied) ? Unpremultiply
                                                  : NoPremultiplicationOp;

    converter.run(srcFormat, dstFormat, premultiplicationOp);

    if (!converter.Success()) {
        
        
        
        NS_RUNTIMEABORT("programming mistake in WebGL texture conversions");
    }
}

} 
