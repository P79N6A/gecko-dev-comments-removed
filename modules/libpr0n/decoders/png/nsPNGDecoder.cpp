







































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


enum { 
  FRAME_HIDDEN         = 0x01
};

static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr);
static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row,
                           png_uint_32 row_num, int pass);
static void PNGAPI frame_info_callback(png_structp png_ptr, png_uint_32 frame_num);
static void PNGAPI end_callback(png_structp png_ptr, png_infop info_ptr);
static void PNGAPI error_callback(png_structp png_ptr, png_const_charp error_msg);
static void PNGAPI warning_callback(png_structp png_ptr, png_const_charp warning_msg);

#ifdef PR_LOGGING
PRLogModuleInfo *gPNGLog = PR_NewLogModule("PNGDecoder");
#endif

NS_IMPL_ISUPPORTS1(nsPNGDecoder, imgIDecoder)

nsPNGDecoder::nsPNGDecoder() :
  mPNG(nsnull), mInfo(nsnull),
  mCMSLine(nsnull), interlacebuf(nsnull),
  mInProfile(nsnull), mTransform(nsnull),
  ibpr(0), apngFlags(0), mChannels(0), mError(PR_FALSE)
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
}


void nsPNGDecoder::SetAnimFrameInfo()
{
  png_uint_16 delay_num, delay_den; 
  png_byte dispose_op;
  PRInt32 timeout; 
  
  delay_num = png_get_next_frame_delay_num(mPNG, mInfo);
  delay_den = png_get_next_frame_delay_den(mPNG, mInfo);
  dispose_op = png_get_next_frame_dispose_op(mPNG, mInfo);

  

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
}





NS_IMETHODIMP nsPNGDecoder::Init(imgILoad *aLoad)
{
#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
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
  
  png_set_keep_unknown_chunks(mPNG, 1, unused_chunks,
     (int)sizeof(unused_chunks)/5);   
#endif

  
  png_set_progressive_read_fn(mPNG, static_cast<png_voidp>(this),
                              info_callback, row_callback, end_callback);

  return NS_OK;
}


NS_IMETHODIMP nsPNGDecoder::Close()
{
  if (mPNG)
    png_destroy_read_struct(&mPNG, mInfo ? &mInfo : NULL, NULL);

  return NS_OK;
}


NS_IMETHODIMP nsPNGDecoder::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
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

  
  if (setjmp(decoder->mPNG->jmpbuf)) {
    png_destroy_read_struct(&decoder->mPNG, &decoder->mInfo, NULL);

    decoder->mError = PR_TRUE;
    *writeCount = 0;
    return NS_ERROR_FAILURE;
  }

  png_process_data(decoder->mPNG, decoder->mInfo,
                   reinterpret_cast<unsigned char *>(const_cast<char *>(fromRawSegment)), count);

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

#ifdef DEBUG_tor
    fprintf(stderr, "PNG profileSpace: 0x%08X\n", profileSpace);
#endif

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
      png_get_sRGB(png_ptr, info_ptr, &fileIntent);
      PRUint32 map[] = { INTENT_PERCEPTUAL, INTENT_RELATIVE_COLORIMETRIC,
                         INTENT_SATURATION, INTENT_ABSOLUTE_COLORIMETRIC };
      *intent = map[fileIntent];
    }
  }

  
  if (!profile && png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)) {
    cmsCIExyY whitePoint = {0.3127, 0.3290, 1.0};         
    cmsCIExyYTRIPLE primaries = {
      {0.6400, 0.3300, 1.0},
      {0.3000, 0.6000, 1.0},
      {0.1500, 0.0600, 1.0}
    };

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)) {
      png_get_cHRM(png_ptr, info_ptr,
                   &whitePoint.x, &whitePoint.y,
                   &primaries.Red.x,   &primaries.Red.y,
                   &primaries.Green.x, &primaries.Green.y,
                   &primaries.Blue.x,  &primaries.Blue.y);

      whitePoint.Y =
        primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;
    }

    double gammaOfFile;
    LPGAMMATABLE gammaTable[3];

    png_get_gAMA(png_ptr, info_ptr, &gammaOfFile);

    gammaTable[0] = gammaTable[1] = gammaTable[2] =
      cmsBuildGamma(256, 1/gammaOfFile);

    if (!gammaTable[0])
      return nsnull;

    profile = cmsCreateRGBProfile(&whitePoint, &primaries, gammaTable);

    if (profile && !(color_type & PNG_COLOR_MASK_COLOR))
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
  int channels;
  double aGamma;

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
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);
    png_set_expand(png_ptr);
  }

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  PRUint32 inType, intent;
  if (gfxPlatform::IsCMSEnabled()) {
    decoder->mInProfile = PNGGetColorProfile(png_ptr, info_ptr,
                                             color_type, &inType, &intent);
  }
  if (decoder->mInProfile && gfxPlatform::GetCMSOutputProfile()) {
    PRUint32 outType;

    if (color_type & PNG_COLOR_MASK_ALPHA || trans)
      outType = TYPE_RGBA_8;
    else
      outType = TYPE_RGB_8;

    decoder->mTransform = cmsCreateTransform(decoder->mInProfile,
                                             inType,
                                             gfxPlatform::GetCMSOutputProfile(),
                                             outType,
                                             intent,
                                             0);
  } else {
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);
    if (gfxPlatform::IsCMSEnabled()) {
      if (color_type & PNG_COLOR_MASK_ALPHA || trans)
        decoder->mTransform = gfxPlatform::GetCMSRGBATransform();
      else
        decoder->mTransform = gfxPlatform::GetCMSRGBTransform();
    }
  }

  if (!decoder->mTransform) {
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

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

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

  if (png_get_gAMA(png_ptr, info_ptr, &aGamma)) {
      if ((aGamma <= 0.0) || (aGamma > 21474.83)) {
          aGamma = 0.45455;
          png_set_gAMA(png_ptr, info_ptr, aGamma);
      }
      png_set_gamma(png_ptr, 2.2, aGamma);
  }
  else
      png_set_gamma(png_ptr, 2.2, 0.45455);

  
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

  decoder->mImage = do_CreateInstance("@mozilla.org/image/container;1");
  if (!decoder->mImage)
    longjmp(decoder->mPNG->jmpbuf, 5); 

  decoder->mImageLoad->SetImage(decoder->mImage);

  decoder->mImage->Init(width, height, decoder->mObserver);

  if (decoder->mObserver)
    decoder->mObserver->OnStartContainer(nsnull, decoder->mImage);

  if (channels == 1 || channels == 3) {
    decoder->format = gfxIFormats::RGB;
  } else if (channels == 2 || channels == 4) {
    if (alpha_bits == 8) {
      decoder->mImage->GetPreferredAlphaChannelFormat(&(decoder->format));
    } else if (alpha_bits == 1) {
      decoder->format = gfxIFormats::RGB_A1;
    }
  }

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL))
    png_set_progressive_frame_fn(png_ptr, frame_info_callback, NULL);
  
  if (png_get_first_frame_is_hidden(png_ptr, info_ptr)) {
    decoder->apngFlags |= FRAME_HIDDEN;
    
    
    decoder->mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
    if (!decoder->mFrame)
      longjmp(png_ptr->jmpbuf, 5); 
    nsresult rv = decoder->mFrame->Init(0, 0, width, height, decoder->format, 24);
    if (NS_FAILED(rv))
      longjmp(png_ptr->jmpbuf, 5); 
  } else {
    decoder->CreateFrame(0, 0, width, height, decoder->format);
  }
  
  PRUint32 bpr;
  decoder->mFrame->GetImageBytesPerRow(&bpr);

  if (decoder->mTransform &&
      (channels <= 2 || interlace_type == PNG_INTERLACE_ADAM7)) {
    PRUint32 bpp[] = { 0, 3, 4, 3, 4 };
    decoder->mCMSLine =
      (PRUint8 *)nsMemory::Alloc(bpp[channels] * width);
    if (!decoder->mCMSLine)
      longjmp(decoder->mPNG->jmpbuf, 5); 
  }

  if (interlace_type == PNG_INTERLACE_ADAM7) {
      decoder->ibpr = channels * width;
    decoder->interlacebuf = (PRUint8 *)nsMemory::Alloc(decoder->ibpr*height);
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
  
  
  
  if (decoder->apngFlags & FRAME_HIDDEN)
    return;

  png_bytep line;
  if (decoder->interlacebuf) {
    line = decoder->interlacebuf+(row_num*decoder->ibpr);
    png_progressive_combine_row(png_ptr, line, new_row);
  }
  else
    line = new_row;

  if (new_row) {
    PRInt32 width;
    decoder->mFrame->GetWidth(&width);
    PRUint32 iwidth = width;

    gfx_format format;
    decoder->mFrame->GetFormat(&format);

    
    PRUint8 *imageData;
    PRUint32 imageDataLength, bpr = width * sizeof(PRUint32);
    decoder->mFrame->GetImageData(&imageData, &imageDataLength);
    PRUint32 *cptr32 = (PRUint32*)(imageData + (row_num*bpr));

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

    switch (format) {
    case gfxIFormats::RGB:
    case gfxIFormats::BGR:
      {
        for (PRUint32 x=iwidth; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(0xFF, line[0], line[1], line[2]);
          line += 3;
        }
      }
      break;
    case gfxIFormats::RGB_A1:
    case gfxIFormats::BGR_A1:
      {
        for (PRUint32 x=iwidth; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(line[3]?0xFF:0x00, line[0], line[1], line[2]);
          line += 4;
        }
      }
      break;
    case gfxIFormats::RGB_A8:
    case gfxIFormats::BGR_A8:
      {
        for (PRUint32 x=width; x>0; --x) {
          *cptr32++ = GFX_PACKED_PIXEL(line[3], line[0], line[1], line[2]);
          line += 4;
        }
      }
      break;
    }

    nsIntRect r(0, row_num, width, 1);
    nsCOMPtr<nsIImage> img(do_GetInterface(decoder->mFrame));
    img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);
    decoder->mObserver->OnDataAvailable(nsnull, decoder->mFrame, &r);
  }
}


void
frame_info_callback(png_structp png_ptr, png_uint_32 frame_num)
{
  png_uint_32 x_offset, y_offset;
  PRInt32 width, height;
  
  nsPNGDecoder *decoder = static_cast<nsPNGDecoder*>(png_get_progressive_ptr(png_ptr));
  
  
  if (!(decoder->apngFlags & FRAME_HIDDEN)) {
    PRInt32 timeout;
    decoder->mFrame->GetTimeout(&timeout);
    decoder->mImage->EndFrameDecode(frame_num, timeout);
    decoder->mObserver->OnStopFrame(nsnull, decoder->mFrame);
  }
  
  decoder->apngFlags &= ~FRAME_HIDDEN;
  
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
  
  if (!(decoder->apngFlags & FRAME_HIDDEN)) {
    PRInt32 timeout;
    decoder->mFrame->GetTimeout(&timeout);
    decoder->mImage->EndFrameDecode(decoder->mPNG->num_frames_read, timeout);
  }
  
  decoder->mImage->DecodingComplete();
  
  if (decoder->mObserver) {
    if (!(decoder->apngFlags & FRAME_HIDDEN))
      decoder->mObserver->OnStopFrame(nsnull, decoder->mFrame);
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
  
  if (strncmp(warning_msg, "Missing PLTE before tRNS", 24) == 0)
    png_error(png_ptr, warning_msg);
  PR_LOG(gPNGLog, PR_LOG_WARNING, ("libpng warning: %s\n", warning_msg));
}
