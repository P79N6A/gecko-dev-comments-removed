








































#include "nsPNGDecoder.h"

#include "nsMemory.h"
#include "nsRect.h"

#include "nsIComponentManager.h"
#include "nsIInputStream.h"

#include "imgIContainerObserver.h"
#include "nsIImage.h"
#include "nsIInterfaceRequestorUtils.h"

#include "gfxColor.h"
#include "nsColor.h"

#include "nspr.h"
#include "png.h"

#include "gfxPlatform.h"

static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr);
static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row,
                           png_uint_32 row_num, int pass);
static void PNGAPI frame_info_callback(png_structp png_ptr, png_uint_32 frame_num);
static void PNGAPI end_callback(png_structp png_ptr, png_infop info_ptr);
static void PNGAPI error_callback(png_structp png_ptr, png_const_charp error_msg);
static void PNGAPI warning_callback(png_structp png_ptr, png_const_charp warning_msg);

#ifdef PR_LOGGING
static PRLogModuleInfo *gPNGLog = PR_NewLogModule("PNGDecoder");
static PRLogModuleInfo *gPNGDecoderAccountingLog = PR_NewLogModule("PNGDecoderAccounting");
#endif

NS_IMPL_ISUPPORTS1(nsPNGDecoder, imgIDecoder)

nsPNGDecoder::nsPNGDecoder() :
  mPNG(nsnull), mInfo(nsnull),
  mCMSLine(nsnull), interlacebuf(nsnull),
  mInProfile(nsnull), mTransform(nsnull),
  mChannels(0), mError(PR_FALSE), mFrameIsHidden(PR_FALSE)
{
}

nsPNGDecoder::~nsPNGDecoder()
{
  if (mCMSLine)
    nsMemory::Free(mCMSLine);
  if (interlacebuf)
    nsMemory::Free(interlacebuf);
  if (mInProfile) {
    cmsCloseProfile(mInProfile);

    
    if (mTransform)
      cmsDeleteTransform(mTransform);
  }
}


void nsPNGDecoder::CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset, 
                                PRInt32 width, PRInt32 height, gfx_format format)
{
  mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  if (!mFrame)
    longjmp(mPNG->jmpbuf, 5); 

  nsresult rv = mFrame->Init(x_offset, y_offset, width, height, format, 24);
  if (NS_FAILED(rv))
    longjmp(mPNG->jmpbuf, 5); 

  if (png_get_valid(mPNG, mInfo, PNG_INFO_acTL))
    SetAnimFrameInfo();
  
  mImage->AppendFrame(mFrame);
  
  if (mObserver)
    mObserver->OnStartFrame(nsnull, mFrame);

 
  PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
         ("PNGDecoderAccounting: nsPNGDecoder::CreateFrame -- created image frame with %dx%d pixels in container %p",
          width, height,
          mImage.get ()));

  mFrameHasNoAlpha = PR_TRUE;
}


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
  mFrame->SetTimeout(timeout);
  
  if (dispose_op == PNG_DISPOSE_OP_PREVIOUS)
      mFrame->SetFrameDisposalMethod(imgIContainer::kDisposeRestorePrevious);
  else if (dispose_op == PNG_DISPOSE_OP_BACKGROUND)
      mFrame->SetFrameDisposalMethod(imgIContainer::kDisposeClear);
  else
      mFrame->SetFrameDisposalMethod(imgIContainer::kDisposeKeep);
  
  if (blend_op == PNG_BLEND_OP_SOURCE)
      mFrame->SetBlendMethod(imgIContainer::kBlendSource);
  

}


void nsPNGDecoder::EndImageFrame()
{
  if (mFrameHasNoAlpha) {
    nsCOMPtr<nsIImage> img(do_GetInterface(mFrame));
    img->SetHasNoAlpha();
  }

  
  PRInt32 timeout = 100;
  PRUint32 numFrames = 0;
  mFrame->GetTimeout(&timeout);
  mImage->GetNumFrames(&numFrames);

  
  if (numFrames > 1) {
    
    PRInt32 width, height;
    mFrame->GetWidth(&width);
    mFrame->GetHeight(&height);

    nsIntRect r(0, 0, width, height);
    nsCOMPtr<nsIImage> img(do_GetInterface(mFrame));
    if (NS_FAILED(img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r))) {
      mError = PR_TRUE;
      
    }
    mObserver->OnDataAvailable(nsnull, mFrame, &r);
  }

  mImage->EndFrameDecode(numFrames, timeout);
  if (mObserver)
    mObserver->OnStopFrame(nsnull, mFrame);
}





NS_IMETHODIMP nsPNGDecoder::Init(imgILoad *aLoad)
{
#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
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

  mImageLoad = aLoad;
  mObserver = do_QueryInterface(aLoad);  

  

  
  

  mPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
                                NULL, error_callback, warning_callback);
  if (!mPNG) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mInfo = png_create_info_struct(mPNG);
  if (!mInfo) {
    png_destroy_read_struct(&mPNG, NULL, NULL);
    return NS_ERROR_OUT_OF_MEMORY;
  }

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
  
  if (gfxPlatform::GetCMSMode() == eCMSMode_Off) {
    png_set_keep_unknown_chunks(mPNG, 1, color_chunks, 2);
  }
  png_set_keep_unknown_chunks(mPNG, 1, unused_chunks,
     (int)sizeof(unused_chunks)/5);   
#endif

  
  png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              info_callback, row_callback, end_callback);


  


  mImageLoad->GetImage(getter_AddRefs(mImage));
  if (!mImage) {
    mImage = do_CreateInstance("@mozilla.org/image/container;1");
    if (!mImage)
      return NS_ERROR_OUT_OF_MEMORY;
      
    mImageLoad->SetImage(mImage);
    if (NS_FAILED(mImage->SetDiscardable("image/png"))) {
      PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
             ("PNGDecoderAccounting: info_callback(): failed to set image container %p as discardable",
              mImage.get()));
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsPNGDecoder::Close()
{
  if (mPNG)
    png_destroy_read_struct(&mPNG, mInfo ? &mInfo : NULL, NULL);

  if (mImage) { 
    nsresult result = mImage->RestoreDataDone();
    if (NS_FAILED(result)) {
        PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
            ("PNGDecoderAccounting: nsPNGDecoder::Close(): failure in RestoreDataDone() for image container %p",
                mImage.get()));

        mError = PR_TRUE;
        return result;
    }

    PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
            ("PNGDecoderAccounting: nsPNGDecoder::Close(): image container %p is now with RestoreDataDone",
            mImage.get()));
  }
  return NS_OK;
}


NS_IMETHODIMP nsPNGDecoder::Flush()
{
  return NS_OK;
}


static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(closure);

  if (decoder->mError) {
    *writeCount = 0;
    return NS_ERROR_FAILURE;
  }

  
  
  nsresult result = decoder->mImage->AddRestoreData(const_cast<char *>(fromRawSegment), count);
  if (NS_FAILED (result)) {
    PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
           ("PNGDecoderAccounting: ReadDataOut(): failed to add restore data to image container %p",
            decoder->mImage.get()));

    decoder->mError = PR_TRUE;
    *writeCount = 0;
    return result;
  }

  
  if (setjmp(decoder->mPNG->jmpbuf)) {
    png_destroy_read_struct(&decoder->mPNG, &decoder->mInfo, NULL);

    decoder->mError = PR_TRUE;
    *writeCount = 0;
    return NS_ERROR_FAILURE;
  }
  png_process_data(decoder->mPNG, decoder->mInfo,
                   reinterpret_cast<unsigned char *>(const_cast<char *>(fromRawSegment)), count);

  PR_LOG(gPNGDecoderAccountingLog, PR_LOG_DEBUG,
         ("PNGDecoderAccounting: ReadDataOut(): Added restore data to image container %p",
          decoder->mImage.get()));

  *writeCount = count;
  return NS_OK;
}



NS_IMETHODIMP nsPNGDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  NS_ASSERTION(inStr, "Got a null input stream!");

  nsresult rv;

  if (!mError)
    rv = inStr->ReadSegments(ReadDataOut, this, count, _retval);

  if (mError) {
    *_retval = 0;
    rv = NS_ERROR_FAILURE;
  }

  return rv;
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


static cmsHPROFILE
PNGGetColorProfile(png_structp png_ptr, png_infop info_ptr,
                   int color_type, PRUint32 *inType, PRUint32 *intent)
{
  cmsHPROFILE profile = nsnull;
  *intent = INTENT_PERCEPTUAL; 

  
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)) {
    png_uint_32 profileLen;
    char *profileData, *profileName;
    int compression;

    png_get_iCCP(png_ptr, info_ptr, &profileName, &compression,
                 &profileData, &profileLen);

    profile = cmsOpenProfileFromMem(profileData, profileLen);
    PRUint32 profileSpace = cmsGetColorSpace(profile);

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
      cmsCloseProfile(profile);
      profile = nsnull;
    } else {
      *intent = cmsTakeRenderingIntent(profile);
    }
  }

  
  if (!profile && png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)) {
    profile = cmsCreate_sRGBProfile();

    if (profile) {
      int fileIntent;
      png_set_gray_to_rgb(png_ptr); 
      png_get_sRGB(png_ptr, info_ptr, &fileIntent);
      PRUint32 map[] = { INTENT_PERCEPTUAL, INTENT_RELATIVE_COLORIMETRIC,
                         INTENT_SATURATION, INTENT_ABSOLUTE_COLORIMETRIC };
      *intent = map[fileIntent];
    }
  }

  
  if (!profile && 
       png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA) &&
       png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)) {
    cmsCIExyYTRIPLE primaries;
    cmsCIExyY whitePoint;

    png_get_cHRM(png_ptr, info_ptr,
                 &whitePoint.x, &whitePoint.y,
                 &primaries.Red.x,   &primaries.Red.y,
                 &primaries.Green.x, &primaries.Green.y,
                 &primaries.Blue.x,  &primaries.Blue.y);
    whitePoint.Y =
      primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;

    double gammaOfFile;
    LPGAMMATABLE gammaTable[3];

    png_get_gAMA(png_ptr, info_ptr, &gammaOfFile);

    gammaTable[0] = gammaTable[1] = gammaTable[2] =
      cmsBuildGamma(256, 1/gammaOfFile);

    if (!gammaTable[0])
      return nsnull;

    profile = cmsCreateRGBProfile(&whitePoint, &primaries, gammaTable);

    if (profile)
      png_set_gray_to_rgb(png_ptr);

    cmsFreeGamma(gammaTable[0]);
  }

  if (profile) {
    PRUint32 profileSpace = cmsGetColorSpace(profile);
    if (profileSpace == icSigGrayData) {
      if (color_type & PNG_COLOR_MASK_ALPHA)
        *inType = TYPE_GRAYA_8;
      else
        *inType = TYPE_GRAY_8;
    } else {
      if (color_type & PNG_COLOR_MASK_ALPHA ||
          png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        *inType = TYPE_RGBA_8;
      else
        *inType = TYPE_RGB_8;
    }
  }

  return profile;
}

void
info_callback(png_structp png_ptr, png_infop info_ptr)
{

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_type;
  unsigned int channels;

  png_bytep trans = NULL;
  int num_trans = 0;

  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));

  
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, &compression_type, &filter_type);
  
  
#define MOZ_PNG_MAX_DIMENSION 1000000L
  if (width > MOZ_PNG_MAX_DIMENSION || height > MOZ_PNG_MAX_DIMENSION)
    longjmp(decoder->mPNG->jmpbuf, 1);
#undef MOZ_PNG_MAX_DIMENSION

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

  PRUint32 inType, intent, pIntent;
  if (gfxPlatform::GetCMSMode() != eCMSMode_Off) {
    intent = gfxPlatform::GetRenderingIntent();
    decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                             color_type, &inType, &pIntent);
    
    if (intent == -1)
      intent = pIntent;
  }
  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    PRUint32 outType;
    PRUint32 dwFlags = 0;

    if (color_type & PNG_COLOR_MASK_ALPHA || num_trans)
      outType = TYPE_RGBA_8;
    else
      outType = TYPE_RGB_8;

    
    if ((inType == outType) && 
        ((inType == TYPE_RGB_8) || (inType == TYPE_RGBA_8)))
      dwFlags |= cmsFLAGS_FLOATSHAPER;

    decoder->mTransform = cmsCreateTransform(decoder->mInProfile,
                                             inType,
                                             gfxPlatform::GetCMSOutputProfile(),
                                             outType,
                                             intent,
                                             dwFlags);
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

  if (decoder->mObserver)
    decoder->mObserver->OnStartDecode(nsnull);

  


  PRInt32 containerWidth, containerHeight;
  decoder->mImage->GetWidth(&containerWidth);
  decoder->mImage->GetHeight(&containerHeight);
  if (containerWidth == 0 && containerHeight == 0) {
    
    decoder->mImage->Init(width, height, decoder->mObserver);
  } else if (containerWidth != width || containerHeight != height) {
    longjmp(decoder->mPNG->jmpbuf, 5); 
  }

  if (decoder->mObserver)
    decoder->mObserver->OnStartContainer(nsnull, decoder->mImage);

  if (channels == 1 || channels == 3) {
    decoder->format = gfxIFormats::RGB;
  } else if (channels == 2 || channels == 4) {
    if (alpha_bits == 8) {
      decoder->format = gfxIFormats::RGB_A8;
    } else if (alpha_bits == 1) {
      decoder->format = gfxIFormats::RGB_A1;
    }
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL))
    png_set_progressive_frame_fn(png_ptr, frame_info_callback, NULL);
  
  if (png_get_first_frame_is_hidden(png_ptr, info_ptr)) {
    decoder->mFrameIsHidden = PR_TRUE;
  } else {
    decoder->CreateFrame(0, 0, width, height, decoder->format);
  }
  
  if (decoder->mTransform &&
      (channels <= 2 || interlace_type == PNG_INTERLACE_ADAM7)) {
    PRUint32 bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      (PRUint8 *)nsMemory::Alloc(bpp[channels] * width);
    if (!decoder->mCMSLine)
      longjmp(decoder->mPNG->jmpbuf, 5); 
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
    if (height < PR_INT32_MAX / (width * channels))
      decoder->interlacebuf = (PRUint8 *)nsMemory::Alloc(channels * width * height);
    if (!decoder->interlacebuf) {
      longjmp(decoder->mPNG->jmpbuf, 5); 
    }
  }
  
  if (png_get_first_frame_is_hidden(png_ptr, info_ptr))
    decoder->mFrame = nsnull;
  
  return;
}

void
row_callback(png_structp png_ptr, png_bytep new_row,
             png_uint_32 row_num, int pass)
{
  


























  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));
  
  
  if (decoder->mFrameIsHidden)
    return;

  if (new_row) {
    PRInt32 width;
    decoder->mFrame->GetWidth(&width);
    PRUint32 iwidth = width;

    png_bytep line = new_row;
    if (decoder->interlacebuf) {
      line = decoder->interlacebuf + (row_num * decoder->mChannels * width);
      png_progressive_combine_row(png_ptr, line, new_row);
    }

    
    PRUint8 *imageData;
    PRUint32 imageDataLength, bpr = width * sizeof(PRUint32);
    decoder->mFrame->GetImageData(&imageData, &imageDataLength);
    PRUint32 *cptr32 = (PRUint32*)(imageData + (row_num*bpr));
    PRBool rowHasNoAlpha = PR_TRUE;

    if (decoder->mTransform) {
      if (decoder->mCMSLine) {
        cmsDoTransform(decoder->mTransform, line, decoder->mCMSLine, iwidth);
        
        PRUint32 channels = decoder->mChannels;
        if (channels == 2 || channels == 4) {
          for (PRUint32 i = 0; i < iwidth; i++)
            decoder->mCMSLine[4 * i + 3] = line[channels * i + channels - 1];
        }
        line = decoder->mCMSLine;
      } else {
        cmsDoTransform(decoder->mTransform, line, line, iwidth);
       }
     }

    switch (decoder->format) {
    case gfxIFormats::RGB:
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
    case gfxIFormats::RGB_A1:
      {
        for (PRUint32 x=iwidth; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(line[3]?0xFF:0x00, line[0], line[1], line[2]);
          if (line[3] == 0)
            rowHasNoAlpha = PR_FALSE;
          line += 4;
        }
      }
      break;
    case gfxIFormats::RGB_A8:
      {
        for (PRUint32 x=width; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(line[3], line[0], line[1], line[2]);
          if (line[3] != 0xff)
            rowHasNoAlpha = PR_FALSE;
          line += 4;
        }
      }
      break;
    }

    if (!rowHasNoAlpha)
      decoder->mFrameHasNoAlpha = PR_FALSE;

    PRUint32 numFrames = 0;
    decoder->mImage->GetNumFrames(&numFrames);
    if (numFrames <= 1) {
      
      nsIntRect r(0, row_num, width, 1);
      nsCOMPtr<nsIImage> img(do_GetInterface(decoder->mFrame));
      if (NS_FAILED(img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r))) {
        decoder->mError = PR_TRUE;  
        return;
      }
      decoder->mObserver->OnDataAvailable(nsnull, decoder->mFrame, &r);
    }
  }
}


void
frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
{
  png_uint_32 x_offset, y_offset;
  PRInt32 width, height;
  
  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));
  
  
  if (!decoder->mFrameIsHidden)
    decoder->EndImageFrame();
  
  decoder->mFrameIsHidden = PR_FALSE;
  
  x_offset = png_get_next_frame_x_offset(png_ptr, decoder->mInfo);
  y_offset = png_get_next_frame_y_offset(png_ptr, decoder->mInfo);
  width = png_get_next_frame_width(png_ptr, decoder->mInfo);
  height = png_get_next_frame_height(png_ptr, decoder->mInfo);
  
  decoder->CreateFrame(x_offset, y_offset, width, height, decoder->format);
}

void
end_callback(png_structp png_ptr, png_infop info_ptr)
{
  











  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));
  
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL)) {
    PRInt32 num_plays = png_get_num_plays(png_ptr, info_ptr);
    decoder->mImage->SetLoopCount(num_plays - 1);
  }
  
  if (!decoder->mFrameIsHidden)
    decoder->EndImageFrame();
  
  decoder->mImage->DecodingComplete();

  if (decoder->mObserver) {
    decoder->mObserver->OnStopContainer(nsnull, decoder->mImage);
    decoder->mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }
}


void
error_callback(png_structp png_ptr, png_const_charp error_msg)
{
  PR_LOG(gPNGLog, PR_LOG_ERROR, ("libpng error: %s\n", error_msg));
  longjmp(png_ptr->jmpbuf, 1);
}


void
warning_callback(png_structp png_ptr, png_const_charp warning_msg)
{
  PR_LOG(gPNGLog, PR_LOG_WARNING, ("libpng warning: %s\n", warning_msg));
}
