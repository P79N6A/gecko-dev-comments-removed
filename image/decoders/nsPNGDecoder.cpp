





#include "ImageLogging.h" 
#include "gfxColor.h"
#include "gfxPlatform.h"
#include "nsColor.h"
#include "nsIInputStream.h"
#include "nsMemory.h"
#include "nsPNGDecoder.h"
#include "nsRect.h"
#include "nspr.h"
#include "png.h"
#include "RasterImage.h"
#include "mozilla/Telemetry.h"

#include <algorithm>

namespace mozilla {
namespace image {

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetPNGLog()
{
  static PRLogModuleInfo* sPNGLog;
  if (!sPNGLog) {
    sPNGLog = PR_NewLogModule("PNGDecoder");
  }
  return sPNGLog;
}

static PRLogModuleInfo*
GetPNGDecoderAccountingLog()
{
  static PRLogModuleInfo* sPNGDecoderAccountingLog;
  if (!sPNGDecoderAccountingLog) {
    sPNGDecoderAccountingLog = PR_NewLogModule("PNGDecoderAccounting");
  }
  return sPNGDecoderAccountingLog;
}
#endif


#ifndef MOZ_PNG_MAX_DIMENSION
#  define MOZ_PNG_MAX_DIMENSION 32767
#endif


#define WIDTH_OFFSET 16
#define HEIGHT_OFFSET (WIDTH_OFFSET + 4)
#define BYTES_NEEDED_FOR_DIMENSIONS (HEIGHT_OFFSET + 4)

nsPNGDecoder::AnimFrameInfo::AnimFrameInfo()
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{ }

#ifdef PNG_APNG_SUPPORTED
nsPNGDecoder::AnimFrameInfo::AnimFrameInfo(png_structp aPNG, png_infop aInfo)
 : mDispose(DisposalMethod::KEEP)
 , mBlend(BlendMethod::OVER)
 , mTimeout(0)
{
  png_uint_16 delay_num, delay_den;
  
  png_byte dispose_op;
  png_byte blend_op;
  delay_num = png_get_next_frame_delay_num(aPNG, aInfo);
  delay_den = png_get_next_frame_delay_den(aPNG, aInfo);
  dispose_op = png_get_next_frame_dispose_op(aPNG, aInfo);
  blend_op = png_get_next_frame_blend_op(aPNG, aInfo);

  if (delay_num == 0) {
    mTimeout = 0; 
  } else {
    if (delay_den == 0) {
      delay_den = 100; 
    }

    
    
    mTimeout = static_cast<int32_t>(static_cast<double>(delay_num) *
                                    1000 / delay_den);
  }

  if (dispose_op == PNG_DISPOSE_OP_PREVIOUS) {
    mDispose = DisposalMethod::RESTORE_PREVIOUS;
  } else if (dispose_op == PNG_DISPOSE_OP_BACKGROUND) {
    mDispose = DisposalMethod::CLEAR;
  } else {
    mDispose = DisposalMethod::KEEP;
  }

  if (blend_op == PNG_BLEND_OP_SOURCE) {
    mBlend = BlendMethod::SOURCE;
  } else {
    mBlend = BlendMethod::OVER;
  }
}
#endif


const uint8_t
nsPNGDecoder::pngSignatureBytes[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

nsPNGDecoder::nsPNGDecoder(RasterImage* aImage)
 : Decoder(aImage),
   mPNG(nullptr), mInfo(nullptr),
   mCMSLine(nullptr), interlacebuf(nullptr),
   mInProfile(nullptr), mTransform(nullptr),
   mHeaderBytesRead(0), mCMSMode(0),
   mChannels(0), mFrameIsHidden(false),
   mDisablePremultipliedAlpha(false),
   mNumFrames(0)
{
}

nsPNGDecoder::~nsPNGDecoder()
{
  if (mPNG) {
    png_destroy_read_struct(&mPNG, mInfo ? &mInfo : nullptr, nullptr);
  }
  if (mCMSLine) {
    free(mCMSLine);
  }
  if (interlacebuf) {
    free(interlacebuf);
  }
  if (mInProfile) {
    qcms_profile_release(mInProfile);

    
    if (mTransform) {
      qcms_transform_release(mTransform);
    }
  }
}


void nsPNGDecoder::CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset,
                               int32_t width, int32_t height,
                               gfx::SurfaceFormat format)
{
  MOZ_ASSERT(HasSize());

  if (format == gfx::SurfaceFormat::B8G8R8A8) {
    PostHasTransparency();
  }

  
  
  nsIntRect neededRect(x_offset, y_offset, width, height);
  nsRefPtr<imgFrame> currentFrame = GetCurrentFrame();
  if (!currentFrame->GetRect().IsEqualEdges(neededRect)) {
    if (mNumFrames == 0) {
      
      
      PostHasTransparency();
    }

    NeedNewFrame(mNumFrames, x_offset, y_offset, width, height, format);
  } else if (mNumFrames != 0) {
    NeedNewFrame(mNumFrames, x_offset, y_offset, width, height, format);
  }

  mFrameRect = neededRect;

  PR_LOG(GetPNGDecoderAccountingLog(), PR_LOG_DEBUG,
         ("PNGDecoderAccounting: nsPNGDecoder::CreateFrame -- created "
          "image frame with %dx%d pixels in container %p",
          width, height,
          &mImage));

#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(mPNG, mInfo, PNG_INFO_acTL)) {
    mAnimInfo = AnimFrameInfo(mPNG, mInfo);

    if (mAnimInfo.mDispose == DisposalMethod::CLEAR) {
      
      
      PostHasTransparency();
    }
  }
#endif
}


void
nsPNGDecoder::EndImageFrame()
{
  if (mFrameIsHidden) {
    return;
  }

  mNumFrames++;

  Opacity opacity = Opacity::SOME_TRANSPARENCY;
  if (format == gfx::SurfaceFormat::B8G8R8X8) {
    opacity = Opacity::OPAQUE;
  }

#ifdef PNG_APNG_SUPPORTED
  uint32_t numFrames = GetFrameCount();

  
  if (numFrames > 1) {
    PostInvalidation(mFrameRect);
  }
#endif

  PostFrameStop(opacity, mAnimInfo.mDispose, mAnimInfo.mTimeout,
                mAnimInfo.mBlend);
}

void
nsPNGDecoder::InitInternal()
{
  
  if (IsSizeDecode()) {
    return;
  }

  mCMSMode = gfxPlatform::GetCMSMode();
  if (GetDecodeFlags() & imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION) {
    mCMSMode = eCMSMode_Off;
  }
  mDisablePremultipliedAlpha =
    GetDecodeFlags() & imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  static png_byte color_chunks[]=
       { 99,  72,  82,  77, '\0',   
        105,  67,  67,  80, '\0'};  
  static png_byte unused_chunks[]=
       { 98,  75,  71,  68, '\0',   
        104,  73,  83,  84, '\0',   
        105,  84,  88, 116, '\0',   
        111,  70,  70, 115, '\0',   
        112,  67,  65,  76, '\0',   
        115,  67,  65,  76, '\0',   
        112,  72,  89, 115, '\0',   
        115,  66,  73,  84, '\0',   
        115,  80,  76,  84, '\0',   
        116,  69,  88, 116, '\0',   
        116,  73,  77,  69, '\0',   
        122,  84,  88, 116, '\0'};  
#endif

  

  
  

  mPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                nullptr, nsPNGDecoder::error_callback,
                                nsPNGDecoder::warning_callback);
  if (!mPNG) {
    PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  mInfo = png_create_info_struct(mPNG);
  if (!mInfo) {
    PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
    png_destroy_read_struct(&mPNG, nullptr, nullptr);
    return;
  }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  
  if (mCMSMode == eCMSMode_Off) {
    png_set_keep_unknown_chunks(mPNG, 1, color_chunks, 2);
  }

  png_set_keep_unknown_chunks(mPNG, 1, unused_chunks,
                              (int)sizeof(unused_chunks)/5);
#endif

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
  if (mCMSMode != eCMSMode_Off) {
    png_set_chunk_malloc_max(mPNG, 4000000L);
  }
#endif

#ifdef PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
#ifndef PR_LOGGING
  
  
  
  
  
  png_set_check_for_invalid_index(mPNG, 0);
#endif
#endif

#if defined(PNG_SET_OPTION_SUPPORTED) && defined(PNG_sRGB_PROFILE_CHECKS) && \
            PNG_sRGB_PROFILE_CHECKS >= 0
  
  png_set_option(mPNG, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);
#endif

  
  png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              nsPNGDecoder::info_callback,
                              nsPNGDecoder::row_callback,
                              nsPNGDecoder::end_callback);

}

void
nsPNGDecoder::WriteInternal(const char* aBuffer, uint32_t aCount)
{
  MOZ_ASSERT(!HasError(), "Shouldn't call WriteInternal after error!");

  
  if (IsSizeDecode()) {

    
    if (mHeaderBytesRead == BYTES_NEEDED_FOR_DIMENSIONS) {
      return;
    }

    
    uint32_t pos = 0;
    const uint8_t* bptr = (uint8_t*)aBuffer;

    while (pos < aCount && mHeaderBytesRead < BYTES_NEEDED_FOR_DIMENSIONS) {
      
      if (mHeaderBytesRead < sizeof(pngSignatureBytes)) {
        if (bptr[pos] != nsPNGDecoder::pngSignatureBytes[mHeaderBytesRead]) {
          PostDataError();
          return;
        }
      }

      
      if ((mHeaderBytesRead >= WIDTH_OFFSET) &&
          (mHeaderBytesRead < BYTES_NEEDED_FOR_DIMENSIONS)) {
        mSizeBytes[mHeaderBytesRead - WIDTH_OFFSET] = bptr[pos];
      }
      pos ++;
      mHeaderBytesRead ++;
    }

    
    if (mHeaderBytesRead == BYTES_NEEDED_FOR_DIMENSIONS) {

      
      uint32_t width = png_get_uint_32(mSizeBytes);
      uint32_t height = png_get_uint_32(mSizeBytes + 4);

      
      if ((width > MOZ_PNG_MAX_DIMENSION) || (height > MOZ_PNG_MAX_DIMENSION)) {
        PostDataError();
        return;
      }

      
      PostSize(width, height);
    }

  
  } else {

    
    if (setjmp(png_jmpbuf(mPNG))) {

      
      
      if (!HasError()) {
        PostDataError();
      }

      png_destroy_read_struct(&mPNG, &mInfo, nullptr);
      return;
    }

    
    png_process_data(mPNG, mInfo, (unsigned char*)aBuffer, aCount);

  }
}



static void
PNGDoGammaCorrection(png_structp png_ptr, png_infop info_ptr)
{
  double aGamma;

  if (png_get_gAMA(png_ptr, info_ptr, &aGamma)) {
    if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
      aGamma = 0.45455;
      png_set_gAMA(png_ptr, info_ptr, aGamma);
    }
    png_set_gamma(png_ptr, 2.2, aGamma);
  } else {
    png_set_gamma(png_ptr, 2.2, 0.45455);
  }
}


static qcms_profile*
PNGGetColorProfile(png_structp png_ptr, png_infop info_ptr,
                   int color_type, qcms_data_type* inType, uint32_t* intent)
{
  qcms_profile* profile = nullptr;
  *intent = QCMS_INTENT_PERCEPTUAL; 

  
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)) {
    png_uint_32 profileLen;
    png_bytep profileData;
    png_charp profileName;
    int compression;

    png_get_iCCP(png_ptr, info_ptr, &profileName, &compression,
                 &profileData, &profileLen);

    profile = qcms_profile_from_memory((char*)profileData, profileLen);
    if (profile) {
      uint32_t profileSpace = qcms_profile_get_color_space(profile);

      bool mismatch = false;
      if (color_type & PNG_COLOR_MASK_COLOR) {
        if (profileSpace != icSigRgbData) {
          mismatch = true;
        }
      } else {
        if (profileSpace == icSigRgbData) {
          png_set_gray_to_rgb(png_ptr);
        } else if (profileSpace != icSigGrayData) {
          mismatch = true;
        }
      }

      if (mismatch) {
        qcms_profile_release(profile);
        profile = nullptr;
      } else {
        *intent = qcms_profile_get_rendering_intent(profile);
      }
    }
  }

  
  if (!profile && png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)) {
    profile = qcms_profile_sRGB();

    if (profile) {
      int fileIntent;
      png_set_gray_to_rgb(png_ptr);
      png_get_sRGB(png_ptr, info_ptr, &fileIntent);
      uint32_t map[] = { QCMS_INTENT_PERCEPTUAL,
                         QCMS_INTENT_RELATIVE_COLORIMETRIC,
                         QCMS_INTENT_SATURATION,
                         QCMS_INTENT_ABSOLUTE_COLORIMETRIC };
      *intent = map[fileIntent];
    }
  }

  
  if (!profile &&
       png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA) &&
       png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)) {
    qcms_CIE_xyYTRIPLE primaries;
    qcms_CIE_xyY whitePoint;

    png_get_cHRM(png_ptr, info_ptr,
                 &whitePoint.x, &whitePoint.y,
                 &primaries.red.x,   &primaries.red.y,
                 &primaries.green.x, &primaries.green.y,
                 &primaries.blue.x,  &primaries.blue.y);
    whitePoint.Y =
      primaries.red.Y = primaries.green.Y = primaries.blue.Y = 1.0;

    double gammaOfFile;

    png_get_gAMA(png_ptr, info_ptr, &gammaOfFile);

    profile = qcms_profile_create_rgb_with_gamma(whitePoint, primaries,
                                                 1.0/gammaOfFile);

    if (profile) {
      png_set_gray_to_rgb(png_ptr);
    }
  }

  if (profile) {
    uint32_t profileSpace = qcms_profile_get_color_space(profile);
    if (profileSpace == icSigGrayData) {
      if (color_type & PNG_COLOR_MASK_ALPHA) {
        *inType = QCMS_DATA_GRAYA_8;
      } else {
        *inType = QCMS_DATA_GRAY_8;
      }
    } else {
      if (color_type & PNG_COLOR_MASK_ALPHA ||
          png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        *inType = QCMS_DATA_RGBA_8;
      } else {
        *inType = QCMS_DATA_RGB_8;
      }
    }
  }

  return profile;
}

void
nsPNGDecoder::info_callback(png_structp png_ptr, png_infop info_ptr)
{

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_type;
  unsigned int channels;

  png_bytep trans = nullptr;
  int num_trans = 0;

  nsPNGDecoder* decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, &compression_type, &filter_type);

  
  if (width > MOZ_PNG_MAX_DIMENSION || height > MOZ_PNG_MAX_DIMENSION) {
    png_longjmp(decoder->mPNG, 1);
  }

  
  decoder->PostSize(width, height);
  if (decoder->HasError()) {
    
    png_longjmp(decoder->mPNG, 1);
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_expand(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand(png_ptr);
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_color_16p trans_values;
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);
    
    
    
    if (bit_depth < 16) {
      png_uint_16 sample_max = (1 << bit_depth) - 1;
      if ((color_type == PNG_COLOR_TYPE_GRAY &&
           trans_values->gray > sample_max) ||
           (color_type == PNG_COLOR_TYPE_RGB &&
           (trans_values->red > sample_max ||
           trans_values->green > sample_max ||
           trans_values->blue > sample_max))) {
        
        png_free_data(png_ptr, info_ptr, PNG_FREE_TRNS, 0);
        num_trans = 0;
      }
    }
    if (num_trans != 0) {
      png_set_expand(png_ptr);
    }
  }

  if (bit_depth == 16) {
    png_set_scale_16(png_ptr);
  }

  qcms_data_type inType = QCMS_DATA_RGBA_8;
  uint32_t intent = -1;
  uint32_t pIntent;
  if (decoder->mCMSMode != eCMSMode_Off) {
    intent = gfxPlatform::GetRenderingIntent();
    decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                             color_type, &inType, &pIntent);
    
    if (intent == uint32_t(-1)) {
      intent = pIntent;
    }
  }
  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    qcms_data_type outType;

    if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
      outType = QCMS_DATA_RGBA_8;
    } else {
      outType = QCMS_DATA_RGB_8;
    }

    decoder->mTransform = qcms_transform_create(decoder->mInProfile,
                                           inType,
                                           gfxPlatform::GetCMSOutputProfile(),
                                           outType,
                                           (qcms_intent)intent);
  } else {
    png_set_gray_to_rgb(png_ptr);

    
    if (decoder->mCMSMode != eCMSMode_Off) {
      PNGDoGammaCorrection(png_ptr, info_ptr);
    }

    if (decoder->mCMSMode == eCMSMode_All) {
      if (color_type & PNG_COLOR_MASK_ALPHA || num_trans) {
        decoder->mTransform = gfxPlatform::GetCMSRGBATransform();
      } else {
        decoder->mTransform = gfxPlatform::GetCMSRGBTransform();
      }
    }
  }

  
  if (interlace_type == PNG_INTERLACE_ADAM7) {
    
    png_set_interlace_handling(png_ptr);
  }

  
  
  png_read_update_info(png_ptr, info_ptr);
  decoder->mChannels = channels = png_get_channels(png_ptr, info_ptr);

  
  
  

  if (channels == 1 || channels == 3) {
    decoder->format = gfx::SurfaceFormat::B8G8R8X8;
  } else if (channels == 2 || channels == 4) {
    decoder->format = gfx::SurfaceFormat::B8G8R8A8;
  }

#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL)) {
    png_set_progressive_frame_fn(png_ptr, nsPNGDecoder::frame_info_callback,
                                 nullptr);
  }

  if (png_get_first_frame_is_hidden(png_ptr, info_ptr)) {
    decoder->mFrameIsHidden = true;
  } else {
#endif
    decoder->CreateFrame(0, 0, width, height, decoder->format);
#ifdef PNG_APNG_SUPPORTED
  }
#endif

  if (decoder->mTransform &&
      (channels <= 2 || interlace_type == PNG_INTERLACE_ADAM7)) {
    uint32_t bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      (uint8_t*)malloc(bpp[channels] * width);
    if (!decoder->mCMSLine) {
      png_longjmp(decoder->mPNG, 5); 
    }
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
    if (height < INT32_MAX / (width * channels)) {
      decoder->interlacebuf = (uint8_t*)malloc(channels * width * height);
    }
    if (!decoder->interlacebuf) {
      png_longjmp(decoder->mPNG, 5); 
    }
  }

  if (decoder->NeedsNewFrame()) {
    
    
    png_process_data_pause(png_ptr,  1);
  }
}

void
nsPNGDecoder::row_callback(png_structp png_ptr, png_bytep new_row,
                           png_uint_32 row_num, int pass)
{
  


























  nsPNGDecoder* decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  if (decoder->mFrameIsHidden) {
    return;
  }

  if (row_num >= (png_uint_32) decoder->mFrameRect.height) {
    return;
  }

  if (new_row) {
    int32_t width = decoder->mFrameRect.width;
    uint32_t iwidth = decoder->mFrameRect.width;

    png_bytep line = new_row;
    if (decoder->interlacebuf) {
      line = decoder->interlacebuf + (row_num * decoder->mChannels * width);
      png_progressive_combine_row(png_ptr, line, new_row);
    }

    uint32_t bpr = width * sizeof(uint32_t);
    uint32_t* cptr32 = (uint32_t*)(decoder->mImageData + (row_num*bpr));

    if (decoder->mTransform) {
      if (decoder->mCMSLine) {
        qcms_transform_data(decoder->mTransform, line, decoder->mCMSLine,
                            iwidth);
        
        uint32_t channels = decoder->mChannels;
        if (channels == 2 || channels == 4) {
          for (uint32_t i = 0; i < iwidth; i++)
            decoder->mCMSLine[4 * i + 3] = line[channels * i + channels - 1];
        }
        line = decoder->mCMSLine;
      } else {
        qcms_transform_data(decoder->mTransform, line, line, iwidth);
      }
    }

    switch (decoder->format) {
      case gfx::SurfaceFormat::B8G8R8X8: {
        
        uint32_t idx = iwidth;

        
        for (; (NS_PTR_TO_UINT32(line) & 0x3) && idx; --idx) {
          *cptr32++ = gfxPackedPixel(0xFF, line[0], line[1], line[2]);
          line += 3;
        }

        
        while (idx >= 4) {
          GFX_BLOCK_RGB_TO_FRGB(line, cptr32);
          idx    -=  4;
          line   += 12;
          cptr32 +=  4;
        }

        
        while (idx--) {
          
          *cptr32++ = gfxPackedPixel(0xFF, line[0], line[1], line[2]);
          line += 3;
        }
      }
      break;
      case gfx::SurfaceFormat::B8G8R8A8: {
        if (!decoder->mDisablePremultipliedAlpha) {
          for (uint32_t x=width; x>0; --x) {
            *cptr32++ = gfxPackedPixel(line[3], line[0], line[1], line[2]);
            line += 4;
          }
        } else {
          for (uint32_t x=width; x>0; --x) {
            *cptr32++ = gfxPackedPixelNoPreMultiply(line[3], line[0], line[1],
                                                    line[2]);
            line += 4;
          }
        }
      }
      break;
      default:
        png_longjmp(decoder->mPNG, 1);
    }

    if (decoder->mNumFrames <= 1) {
      
      
      nsIntRect r(0, row_num, width, 1);
      decoder->PostInvalidation(r);
    }
  }
}

#ifdef PNG_APNG_SUPPORTED

void
nsPNGDecoder::frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
{
  png_uint_32 x_offset, y_offset;
  int32_t width, height;

  nsPNGDecoder* decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  decoder->EndImageFrame();

  
  decoder->mFrameIsHidden = false;

  x_offset = png_get_next_frame_x_offset(png_ptr, decoder->mInfo);
  y_offset = png_get_next_frame_y_offset(png_ptr, decoder->mInfo);
  width = png_get_next_frame_width(png_ptr, decoder->mInfo);
  height = png_get_next_frame_height(png_ptr, decoder->mInfo);

  decoder->CreateFrame(x_offset, y_offset, width, height, decoder->format);

  if (decoder->NeedsNewFrame()) {
    
    
    png_process_data_pause(png_ptr,  1);
  }
}
#endif

void
nsPNGDecoder::end_callback(png_structp png_ptr, png_infop info_ptr)
{
  











  nsPNGDecoder* decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  MOZ_ASSERT(!decoder->HasError(), "Finishing up PNG but hit error!");

  int32_t loop_count = 0;
#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL)) {
    int32_t num_plays = png_get_num_plays(png_ptr, info_ptr);
    loop_count = num_plays - 1;
  }
#endif

  
  decoder->EndImageFrame();
  decoder->PostDecodeDone(loop_count);
}


void
nsPNGDecoder::error_callback(png_structp png_ptr, png_const_charp error_msg)
{
  PR_LOG(GetPNGLog(), PR_LOG_ERROR, ("libpng error: %s\n", error_msg));
  png_longjmp(png_ptr, 1);
}


void
nsPNGDecoder::warning_callback(png_structp png_ptr, png_const_charp warning_msg)
{
  PR_LOG(GetPNGLog(), PR_LOG_WARNING, ("libpng warning: %s\n", warning_msg));
}

Telemetry::ID
nsPNGDecoder::SpeedHistogram()
{
  return Telemetry::IMAGE_DECODE_SPEED_PNG;
}


} 
} 
