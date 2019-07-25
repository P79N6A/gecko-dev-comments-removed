










































#include "nsPNGDecoder.h"

#include "nsMemory.h"
#include "nsRect.h"

#include "nsIInputStream.h"

#include "RasterImage.h"
#include "imgIContainerObserver.h"

#include "gfxColor.h"
#include "nsColor.h"

#include "nspr.h"
#include "png.h"

#include "gfxPlatform.h"

namespace mozilla {
namespace imagelib {

#ifdef PR_LOGGING
static PRLogModuleInfo *gPNGLog = PR_NewLogModule("PNGDecoder");
static PRLogModuleInfo *gPNGDecoderAccountingLog =
                        PR_NewLogModule("PNGDecoderAccounting");
#endif


#define MOZ_PNG_MAX_DIMENSION 1000000L


#define WIDTH_OFFSET 16
#define HEIGHT_OFFSET (WIDTH_OFFSET + 4)
#define BYTES_NEEDED_FOR_DIMENSIONS (HEIGHT_OFFSET + 4)



static const PRUint8 pngSignatureBytes[] =
               { 137, 80, 78, 71, 13, 10, 26, 10 };

nsPNGDecoder::nsPNGDecoder() :
  mPNG(nsnull), mInfo(nsnull),
  mCMSLine(nsnull), interlacebuf(nsnull),
  mInProfile(nsnull), mTransform(nsnull),
  mHeaderBuf(nsnull), mHeaderBytesRead(0),
  mChannels(0), mFrameIsHidden(PR_FALSE),
  mNotifiedDone(PR_FALSE)
{
}

nsPNGDecoder::~nsPNGDecoder()
{
  if (mPNG)
    png_destroy_read_struct(&mPNG, mInfo ? &mInfo : NULL, NULL);
  if (mCMSLine)
    nsMemory::Free(mCMSLine);
  if (interlacebuf)
    nsMemory::Free(interlacebuf);
  if (mInProfile) {
    qcms_profile_release(mInProfile);

    
    if (mTransform)
      qcms_transform_release(mTransform);
  }
  if (mHeaderBuf)
    nsMemory::Free(mHeaderBuf);
}


void nsPNGDecoder::CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset,
                               PRInt32 width, PRInt32 height,
                               gfxASurface::gfxImageFormat format)
{
  PRUint32 imageDataLength;
  nsresult rv = mImage->AppendFrame(x_offset, y_offset, width, height, format,
                                    &mImageData, &imageDataLength);
  if (NS_FAILED(rv))
    longjmp(png_jmpbuf(mPNG), 5); 

  mFrameRect.x = x_offset;
  mFrameRect.y = y_offset;
  mFrameRect.width = width;
  mFrameRect.height = height;

#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(mPNG, mInfo, PNG_INFO_acTL))
    SetAnimFrameInfo();
#endif

  
  PostFrameStart();

  PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
         ("PNGDecoderAccounting: nsPNGDecoder::CreateFrame -- created "
          "image frame with %dx%d pixels in container %p",
          width, height,
          mImage.get ()));

  mFrameHasNoAlpha = PR_TRUE;
}

#ifdef PNG_APNG_SUPPORTED

void nsPNGDecoder::SetAnimFrameInfo()
{
  png_uint_16 delay_num, delay_den;
  
  png_byte dispose_op;
  png_byte blend_op;
  PRInt32 timeout; 

  delay_num = png_get_next_frame_delay_num(mPNG, mInfo);
  delay_den = png_get_next_frame_delay_den(mPNG, mInfo);
  dispose_op = png_get_next_frame_dispose_op(mPNG, mInfo);
  blend_op = png_get_next_frame_blend_op(mPNG, mInfo);

  if (delay_num == 0) {
    timeout = 0; 
  } else {
    if (delay_den == 0)
      delay_den = 100; 

    
    
    timeout = static_cast<PRInt32>
              (static_cast<PRFloat64>(delay_num) * 1000 / delay_den);
  }

  PRUint32 numFrames = mImage->GetNumFrames();

  mImage->SetFrameTimeout(numFrames - 1, timeout);

  if (dispose_op == PNG_DISPOSE_OP_PREVIOUS)
      mImage->SetFrameDisposalMethod(numFrames - 1,
                                     RasterImage::kDisposeRestorePrevious);
  else if (dispose_op == PNG_DISPOSE_OP_BACKGROUND)
      mImage->SetFrameDisposalMethod(numFrames - 1,
                                     RasterImage::kDisposeClear);
  else
      mImage->SetFrameDisposalMethod(numFrames - 1,
                                     RasterImage::kDisposeKeep);

  if (blend_op == PNG_BLEND_OP_SOURCE)
      mImage->SetFrameBlendMethod(numFrames - 1, RasterImage::kBlendSource);
  

}
#endif


void nsPNGDecoder::EndImageFrame()
{
  PRUint32 numFrames = 1;
#ifdef PNG_APNG_SUPPORTED
  numFrames = mImage->GetNumFrames();

  
  if (numFrames > 1) {
    
    if (mFrameHasNoAlpha)
      mImage->SetFrameHasNoAlpha(numFrames - 1);

    PostInvalidation(mFrameRect);
  }
#endif

  mImage->EndFrameDecode(numFrames - 1);
  PostFrameStop();
}

nsresult
nsPNGDecoder::InitInternal()
{

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

  
  if (!IsSizeDecode() && mObserver)
    mObserver->OnStartDecode(nsnull);

  
  if (IsSizeDecode()) {
    mHeaderBuf = (PRUint8 *)nsMemory::Alloc(BYTES_NEEDED_FOR_DIMENSIONS);
    if (!mHeaderBuf) {
      PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
  }

  

  
  

  mPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                NULL, nsPNGDecoder::error_callback,
                                nsPNGDecoder::warning_callback);
  if (!mPNG) {
    PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mInfo = png_create_info_struct(mPNG);
  if (!mInfo) {
    PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
    png_destroy_read_struct(&mPNG, NULL, NULL);
    return NS_ERROR_OUT_OF_MEMORY;
  }

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
  
  if (gfxPlatform::GetCMSMode() == eCMSMode_Off)
    png_set_keep_unknown_chunks(mPNG, 1, color_chunks, 2);

  png_set_keep_unknown_chunks(mPNG, 1, unused_chunks,
                              (int)sizeof(unused_chunks)/5);   
#endif

#ifdef PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
  if (gfxPlatform::GetCMSMode() != eCMSMode_Off)
    png_set_chunk_malloc_max(mPNG, 4000000L);
#endif

  
  png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              nsPNGDecoder::info_callback,
                              nsPNGDecoder::row_callback,
                              nsPNGDecoder::end_callback);


  return NS_OK;
}

nsresult
nsPNGDecoder::FinishInternal()
{

  
  
  if (!IsSizeDecode() && !mNotifiedDone)
    NotifyDone( PR_FALSE);

  return NS_OK;
}

nsresult
nsPNGDecoder::WriteInternal(const char *aBuffer, PRUint32 aCount)
{
  
  PRUint32 width = 0;
  PRUint32 height = 0;

  
  if (IsError())
    goto error;

  
  if (IsSizeDecode()) {

    
    if (mHeaderBytesRead == BYTES_NEEDED_FOR_DIMENSIONS)
      return NS_OK;

    
    PRUint32 bytesToRead = PR_MIN(aCount, BYTES_NEEDED_FOR_DIMENSIONS -
                                  mHeaderBytesRead);
    memcpy(mHeaderBuf + mHeaderBytesRead, aBuffer, bytesToRead);
    mHeaderBytesRead += bytesToRead;

    
    if (mHeaderBytesRead == BYTES_NEEDED_FOR_DIMENSIONS) {

      
      if (memcmp(mHeaderBuf, pngSignatureBytes, sizeof(pngSignatureBytes))) {
        PostDataError();
        goto error;
      }

      
      width = png_get_uint_32(mHeaderBuf + WIDTH_OFFSET);
      height = png_get_uint_32(mHeaderBuf + HEIGHT_OFFSET);

      
      if ((width > MOZ_PNG_MAX_DIMENSION) || (height > MOZ_PNG_MAX_DIMENSION)) {
        PostDataError();
        goto error;
      }

      
      PostSize(width, height);
    }
  }

  
  else {

    
    if (setjmp(png_jmpbuf(mPNG))) {

      
      
      if (!IsError())
        PostDataError();

      png_destroy_read_struct(&mPNG, &mInfo, NULL);
      goto error;
    }

    
    png_process_data(mPNG, mInfo, (unsigned char *)aBuffer, aCount);

  }

  return NS_OK;

  
  error:
  NS_ABORT_IF_FALSE(IsError(), "Should only get here if we flagged an error!");
  return NS_ERROR_FAILURE;
}

void
nsPNGDecoder::NotifyDone(PRBool aSuccess)
{
  
  NS_ABORT_IF_FALSE(!mNotifiedDone, "Calling NotifyDone twice!");

  
  if (!mFrameIsHidden)
    EndImageFrame();
  if (aSuccess)
    mImage->DecodingComplete();
  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, aSuccess ? NS_OK : NS_ERROR_FAILURE,
                            nsnull);
  }

  
  mNotifiedDone = PR_TRUE;
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
  }
  else
    png_set_gamma(png_ptr, 2.2, 0.45455);

}


static qcms_profile *
PNGGetColorProfile(png_structp png_ptr, png_infop info_ptr,
                   int color_type, qcms_data_type *inType, PRUint32 *intent)
{
  qcms_profile *profile = nsnull;
  *intent = QCMS_INTENT_PERCEPTUAL; 

  
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)) {
    png_uint_32 profileLen;
    char *profileData, *profileName;
    int compression;

    png_get_iCCP(png_ptr, info_ptr, &profileName, &compression,
                 &profileData, &profileLen);

    profile = qcms_profile_from_memory(profileData, profileLen);
    if (profile) {
      PRUint32 profileSpace = qcms_profile_get_color_space(profile);

      PRBool mismatch = PR_FALSE;
      if (color_type & PNG_COLOR_MASK_COLOR) {
        if (profileSpace != icSigRgbData)
          mismatch = PR_TRUE;
      } else {
        if (profileSpace == icSigRgbData)
          png_set_gray_to_rgb(png_ptr);
        else if (profileSpace != icSigGrayData)
          mismatch = PR_TRUE;
      }

      if (mismatch) {
        qcms_profile_release(profile);
        profile = nsnull;
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
      PRUint32 map[] = { QCMS_INTENT_PERCEPTUAL,
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

    if (profile)
      png_set_gray_to_rgb(png_ptr);
  }

  if (profile) {
    PRUint32 profileSpace = qcms_profile_get_color_space(profile);
    if (profileSpace == icSigGrayData) {
      if (color_type & PNG_COLOR_MASK_ALPHA)
        *inType = QCMS_DATA_GRAYA_8;
      else
        *inType = QCMS_DATA_GRAY_8;
    } else {
      if (color_type & PNG_COLOR_MASK_ALPHA ||
          png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        *inType = QCMS_DATA_RGBA_8;
      else
        *inType = QCMS_DATA_RGB_8;
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

  png_bytep trans = NULL;
  int num_trans = 0;

  nsPNGDecoder *decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, &compression_type, &filter_type);

  
  if (width > MOZ_PNG_MAX_DIMENSION || height > MOZ_PNG_MAX_DIMENSION)
    longjmp(png_jmpbuf(decoder->mPNG), 1);

  
  decoder->PostSize(width, height);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    int sample_max = (1 << bit_depth);
    png_color_16p trans_values;
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);
    



    if ((color_type == PNG_COLOR_TYPE_GRAY &&
       (int)trans_values->gray > sample_max) ||
       (color_type == PNG_COLOR_TYPE_RGB &&
       ((int)trans_values->red > sample_max ||
       (int)trans_values->green > sample_max ||
       (int)trans_values->blue > sample_max)))
      {
        
        png_free_data(png_ptr, info_ptr, PNG_FREE_TRNS, 0);
      }
    else
      png_set_expand(png_ptr);
  }

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  qcms_data_type inType;
  PRUint32 intent = -1;
  PRUint32 pIntent;
  if (gfxPlatform::GetCMSMode() != eCMSMode_Off) {
    intent = gfxPlatform::GetRenderingIntent();
    decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                             color_type, &inType, &pIntent);
    
    if (intent == PRUint32(-1))
      intent = pIntent;
  }
  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    qcms_data_type outType;

    if (color_type & PNG_COLOR_MASK_ALPHA || num_trans)
      outType = QCMS_DATA_RGBA_8;
    else
      outType = QCMS_DATA_RGB_8;

    decoder->mTransform = qcms_transform_create(decoder->mInProfile,
                                           inType,
                                           gfxPlatform::GetCMSOutputProfile(),
                                           outType,
                                           (qcms_intent)intent);
  } else {
    png_set_gray_to_rgb(png_ptr);
    PNGDoGammaCorrection(png_ptr, info_ptr);

    if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
      if (color_type & PNG_COLOR_MASK_ALPHA || num_trans)
        decoder->mTransform = gfxPlatform::GetCMSRGBATransform();
      else
        decoder->mTransform = gfxPlatform::GetCMSRGBTransform();
    }
  }

  
  if (interlace_type == PNG_INTERLACE_ADAM7) {
    
    png_set_interlace_handling(png_ptr);
  }

  

  png_read_update_info(png_ptr, info_ptr);
  decoder->mChannels = channels = png_get_channels(png_ptr, info_ptr);

  
  
  

  PRInt32 alpha_bits = 1;

  if (channels == 2 || channels == 4) {
    
    if (num_trans) {
      
      if (color_type == PNG_COLOR_TYPE_PALETTE) {
        for (int i=0; i<num_trans; i++) {
          if ((trans[i] != 0) && (trans[i] != 255)) {
            alpha_bits = 8;
            break;
          }
        }
      }
    } else {
      alpha_bits = 8;
    }
  }

  if (channels == 1 || channels == 3)
    decoder->format = gfxASurface::ImageFormatRGB24;
  else if (channels == 2 || channels == 4)
    decoder->format = gfxASurface::ImageFormatARGB32;

#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL))
    png_set_progressive_frame_fn(png_ptr, nsPNGDecoder::frame_info_callback, NULL);

  if (png_get_first_frame_is_hidden(png_ptr, info_ptr)) {
    decoder->mFrameIsHidden = PR_TRUE;
  } else {
#endif
    decoder->CreateFrame(0, 0, width, height, decoder->format);
#ifdef PNG_APNG_SUPPORTED
  }
#endif

  if (decoder->mTransform &&
      (channels <= 2 || interlace_type == PNG_INTERLACE_ADAM7)) {
    PRUint32 bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      (PRUint8 *)nsMemory::Alloc(bpp[channels] * width);
    if (!decoder->mCMSLine) {
      longjmp(png_jmpbuf(decoder->mPNG), 5); 
    }
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
    if (height < PR_INT32_MAX / (width * channels))
      decoder->interlacebuf = (PRUint8 *)nsMemory::Alloc(channels *
                                                         width * height);
    if (!decoder->interlacebuf) {
      longjmp(png_jmpbuf(decoder->mPNG), 5); 
    }
  }

  




  png_set_crc_action(png_ptr, NULL, PNG_CRC_ERROR_QUIT);

  return;
}

void
nsPNGDecoder::row_callback(png_structp png_ptr, png_bytep new_row,
                           png_uint_32 row_num, int pass)
{
  


























  nsPNGDecoder *decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  if (decoder->mFrameIsHidden)
    return;

  if (row_num >= (png_uint_32) decoder->mFrameRect.height)
    return;

  if (new_row) {
    PRInt32 width = decoder->mFrameRect.width;
    PRUint32 iwidth = decoder->mFrameRect.width;

    png_bytep line = new_row;
    if (decoder->interlacebuf) {
      line = decoder->interlacebuf + (row_num * decoder->mChannels * width);
      png_progressive_combine_row(png_ptr, line, new_row);
    }

    PRUint32 bpr = width * sizeof(PRUint32);
    PRUint32 *cptr32 = (PRUint32*)(decoder->mImageData + (row_num*bpr));
    PRBool rowHasNoAlpha = PR_TRUE;

    if (decoder->mTransform) {
      if (decoder->mCMSLine) {
        qcms_transform_data(decoder->mTransform, line, decoder->mCMSLine,
                            iwidth);
        
        PRUint32 channels = decoder->mChannels;
        if (channels == 2 || channels == 4) {
          for (PRUint32 i = 0; i < iwidth; i++)
            decoder->mCMSLine[4 * i + 3] = line[channels * i + channels - 1];
        }
        line = decoder->mCMSLine;
      } else {
        qcms_transform_data(decoder->mTransform, line, line, iwidth);
       }
     }

    switch (decoder->format) {
      case gfxASurface::ImageFormatRGB24:
      {
        
        PRUint32 idx = iwidth;

        
        for (; (NS_PTR_TO_UINT32(line) & 0x3) && idx; --idx) {
          *cptr32++ = GFX_PACKED_PIXEL(0xFF, line[0], line[1], line[2]);
          line += 3;
        }

        
        while (idx >= 4) {
          GFX_BLOCK_RGB_TO_FRGB(line, cptr32);
          idx    -=  4;
          line   += 12;
          cptr32 +=  4;
        }

        
        while (idx--) {
          
          *cptr32++ = GFX_PACKED_PIXEL(0xFF, line[0], line[1], line[2]);
          line += 3;
        }
      }
      break;
      case gfxASurface::ImageFormatARGB32:
      {
        for (PRUint32 x=width; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(line[3], line[0], line[1], line[2]);
          if (line[3] != 0xff)
            rowHasNoAlpha = PR_FALSE;
          line += 4;
        }
      }
      break;
      default:
        longjmp(png_jmpbuf(decoder->mPNG), 1);
    }

    if (!rowHasNoAlpha)
      decoder->mFrameHasNoAlpha = PR_FALSE;

    PRUint32 numFrames = decoder->mImage->GetNumFrames();
    if (numFrames <= 1) {
      
      
      nsIntRect r(0, row_num, width, 1);
      decoder->PostInvalidation(r);
    }
  }
}


void
nsPNGDecoder::frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
{
#ifdef PNG_APNG_SUPPORTED
  png_uint_32 x_offset, y_offset;
  PRInt32 width, height;

  nsPNGDecoder *decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  if (!decoder->mFrameIsHidden)
    decoder->EndImageFrame();

  decoder->mFrameIsHidden = PR_FALSE;

  x_offset = png_get_next_frame_x_offset(png_ptr, decoder->mInfo);
  y_offset = png_get_next_frame_y_offset(png_ptr, decoder->mInfo);
  width = png_get_next_frame_width(png_ptr, decoder->mInfo);
  height = png_get_next_frame_height(png_ptr, decoder->mInfo);

  decoder->CreateFrame(x_offset, y_offset, width, height, decoder->format);
#endif
}

void
nsPNGDecoder::end_callback(png_structp png_ptr, png_infop info_ptr)
{
  











  nsPNGDecoder *decoder =
               static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  NS_ABORT_IF_FALSE(!decoder->IsError(), "Finishing up PNG but hit error!");

#ifdef PNG_APNG_SUPPORTED
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL)) {
    PRInt32 num_plays = png_get_num_plays(png_ptr, info_ptr);
    decoder->mImage->SetLoopCount(num_plays - 1);
  }
#endif

  
  decoder->NotifyDone( PR_TRUE);
}


void
nsPNGDecoder::error_callback(png_structp png_ptr, png_const_charp error_msg)
{
  PR_LOG(gPNGLog, PR_LOG_ERROR, ("libpng error: %s\n", error_msg));
  longjmp(png_jmpbuf(png_ptr), 1);
}


void
nsPNGDecoder::warning_callback(png_structp png_ptr, png_const_charp warning_msg)
{
  PR_LOG(gPNGLog, PR_LOG_WARNING, ("libpng warning: %s\n", warning_msg));
}

} 
} 
