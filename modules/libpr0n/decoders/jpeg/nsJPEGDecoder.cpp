






































#include "nsJPEGDecoder.h"

#include "imgIContainerObserver.h"

#include "nsIComponentManager.h"
#include "nsIInputStream.h"

#include "nspr.h"
#include "nsCRT.h"
#include "ImageLogging.h"
#include "nsIImage.h"
#include "nsIInterfaceRequestorUtils.h"
#include "gfxColor.h"

#include "jerror.h"

#include "gfxPlatform.h"

extern "C" {
#include "iccjpeg.h"
}

NS_IMPL_ISUPPORTS1(nsJPEGDecoder, imgIDecoder)

#if defined(PR_LOGGING)
PRLogModuleInfo *gJPEGlog = PR_NewLogModule("JPEGDecoder");
#else
#define gJPEGlog
#endif


METHODDEF(void) init_source (j_decompress_ptr jd);
METHODDEF(boolean) fill_input_buffer (j_decompress_ptr jd);
METHODDEF(void) skip_input_data (j_decompress_ptr jd, long num_bytes);
METHODDEF(void) term_source (j_decompress_ptr jd);
METHODDEF(void) my_error_exit (j_common_ptr cinfo);


#define MAX_JPEG_MARKER_LENGTH  (((PRUint32)1 << 16) - 1)


nsJPEGDecoder::nsJPEGDecoder()
{
  mState = JPEG_HEADER;
  mReading = PR_TRUE;

  mSamples = nsnull;

  mBytesToSkip = 0;
  memset(&mInfo, 0, sizeof(jpeg_decompress_struct));
  memset(&mSourceMgr, 0, sizeof(mSourceMgr));
  mInfo.client_data = (void*)this;

  mBuffer = nsnull;
  mBufferLen = mBufferSize = 0;

  mBackBuffer = nsnull;
  mBackBufferLen = mBackBufferSize = mBackBufferUnreadLen = 0;

  mInProfile = nsnull;
  mTransform = nsnull;
}

nsJPEGDecoder::~nsJPEGDecoder()
{
  PR_FREEIF(mBuffer);
  PR_FREEIF(mBackBuffer);
  if (mTransform)
    cmsDeleteTransform(mTransform);
  if (mInProfile)
    cmsCloseProfile(mInProfile);
}





NS_IMETHODIMP nsJPEGDecoder::Init(imgILoad *aLoad)
{
  mImageLoad = aLoad;
  mObserver = do_QueryInterface(aLoad);

  
  mInfo.err = jpeg_std_error(&mErr.pub);
  
  mErr.pub.error_exit = my_error_exit;
  
  if (setjmp(mErr.setjmp_buffer)) {
    


    return NS_ERROR_FAILURE;
  }

  
  jpeg_create_decompress(&mInfo);
  
  mInfo.src = &mSourceMgr;

  

  
  mSourceMgr.init_source = init_source;
  mSourceMgr.fill_input_buffer = fill_input_buffer;
  mSourceMgr.skip_input_data = skip_input_data;
  mSourceMgr.resync_to_restart = jpeg_resync_to_restart;
  mSourceMgr.term_source = term_source;

  
  for (PRUint32 m = 0; m < 16; m++)
    jpeg_save_markers(&mInfo, JPEG_APP0 + m, 0xFFFF);

  return NS_OK;
}



NS_IMETHODIMP nsJPEGDecoder::Close()
{
  PR_LOG(gJPEGlog, PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::Close\n", this));

  if (mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER)
    NS_WARNING("Never finished decoding the JPEG.");

  
  mInfo.src = nsnull;

  jpeg_destroy_decompress(&mInfo);

  return NS_OK;
}


NS_IMETHODIMP nsJPEGDecoder::Flush()
{
  LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Flush");

  PRUint32 ret;
  if (mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER && mState != JPEG_ERROR)
    return this->WriteFrom(nsnull, 0, &ret);

  return NS_OK;
}


NS_IMETHODIMP nsJPEGDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  LOG_SCOPE_WITH_PARAM(gJPEGlog, "nsJPEGDecoder::WriteFrom", "count", count);

  if (inStr) {
    if (!mBuffer) {
      mBuffer = (JOCTET *)PR_Malloc(count);
      if (!mBuffer) {
        mState = JPEG_ERROR;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mBufferSize = count;
    } else if (count > mBufferSize) {
      JOCTET *buf = (JOCTET *)PR_Realloc(mBuffer, count);
      if (!buf) {
        mState = JPEG_ERROR;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mBuffer = buf;
      mBufferSize = count;
    }

    nsresult rv = inStr->Read((char*)mBuffer, count, &mBufferLen);
    *_retval = mBufferLen;

    NS_ASSERTION(NS_SUCCEEDED(rv), "nsJPEGDecoder::WriteFrom -- inStr->Read failed");
  }
  

  
  nsresult error_code;
  if ((error_code = setjmp(mErr.setjmp_buffer)) != 0) {
    mState = JPEG_SINK_NON_JPEG_TRAILER;
    if (error_code == NS_ERROR_FAILURE) {
      

      return NS_OK;
    } else {
      


      return error_code;
    }
  }

  PR_LOG(gJPEGlog, PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::WriteFrom -- processing JPEG data\n", this));

  switch (mState) {
  case JPEG_HEADER:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::WriteFrom -- entering JPEG_HEADER case");

    
    if (jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED)
      return NS_OK; 

    JOCTET  *profile;
    PRUint32 profileLength;

    if (gfxPlatform::IsCMSEnabled() &&
        read_icc_profile(&mInfo, &profile, &profileLength) &&
        (mInProfile = cmsOpenProfileFromMem(profile, profileLength)) != NULL) {
      free(profile);

      PRUint32 profileSpace = cmsGetColorSpace(mInProfile);
      PRBool mismatch = PR_FALSE;

#ifdef DEBUG_tor
      fprintf(stderr, "JPEG profileSpace: 0x%08X\n", profileSpace);
#endif
      switch (mInfo.jpeg_color_space) {
      case JCS_GRAYSCALE:
        if (profileSpace == icSigRgbData)
          mInfo.out_color_space = JCS_RGB;
        else if (profileSpace != icSigGrayData)
          mismatch = PR_TRUE;
        break;
      case JCS_RGB:
        if (profileSpace != icSigRgbData)
          mismatch =  PR_TRUE;
        break;
      case JCS_YCbCr:
        if (profileSpace == icSigRgbData)
          mInfo.out_color_space = JCS_RGB;
        else if (profileSpace != icSigYCbCrData)
          mismatch = PR_TRUE;
        break;
      case JCS_CMYK:
      case JCS_YCCK:
        if (profileSpace == icSigCmykData)
          mInfo.out_color_space = JCS_CMYK;
        else
          mismatch = PR_TRUE;
        break;
      default:
        mState = JPEG_ERROR;
        return NS_ERROR_UNEXPECTED;
      }

      if (!mismatch) {
        PRUint32 space, channels;
        switch (mInfo.out_color_space) {
        case JCS_GRAYSCALE:
          space = PT_GRAY;
          channels = 1;
          break;
        case JCS_RGB:
          space = PT_RGB;
          channels = 3;
          break;
        case JCS_YCbCr:
          space = PT_YCbCr;
          channels = 3;
        case JCS_CMYK:
          space = PT_CMYK;
          channels = 4;
          break;
        default:
          mState = JPEG_ERROR;
          return NS_ERROR_UNEXPECTED;
        }

        PRUint32 type =
          COLORSPACE_SH(space) |
          CHANNELS_SH(channels) |
          BYTES_SH(1);

        
        if (mInfo.jpeg_color_space == JCS_CMYK)
          type |= FLAVOR_SH(mInfo.saw_Adobe_marker ? 1 : 0);

        if (gfxPlatform::GetCMSOutputProfile())
          mTransform = cmsCreateTransform(mInProfile,
                                          type,
                                          gfxPlatform::GetCMSOutputProfile(),
                                          TYPE_RGB_8,
                                          cmsTakeRenderingIntent(mInProfile),
                                          0);
      } else {
#ifdef DEBUG_tor
        fprintf(stderr, "ICM profile colorspace mismatch\n");
#endif
      }
    }

    if (!mTransform) {
      switch (mInfo.jpeg_color_space) {
      case JCS_GRAYSCALE:
      case JCS_RGB:
      case JCS_YCbCr:
        mInfo.out_color_space = JCS_RGB;
        break;
      default:
        mState = JPEG_ERROR;
        return NS_ERROR_UNEXPECTED;
        break;
      }
    }

    



    mInfo.buffered_image = jpeg_has_multiple_scans(&mInfo);

    
    jpeg_calc_output_dimensions(&mInfo);

    mObserver->OnStartDecode(nsnull);

    



    mImageLoad->GetImage(getter_AddRefs(mImage));
    if (mImage) {
      PRInt32 width, height;
      mImage->GetWidth(&width);
      mImage->GetHeight(&height);
      if ((width != (PRInt32)mInfo.image_width) ||
          (height != (PRInt32)mInfo.image_height)) {
        mImage = nsnull;
      }
    }

    if (!mImage) {
      mImage = do_CreateInstance("@mozilla.org/image/container;1");
      if (!mImage) {
        mState = JPEG_ERROR;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mImageLoad->SetImage(mImage);
      mImage->Init(mInfo.image_width, mInfo.image_height, mObserver);
    }

    mObserver->OnStartContainer(nsnull, mImage);

    mImage->GetFrameAt(0, getter_AddRefs(mFrame));

    PRBool createNewFrame = PR_TRUE;

    if (mFrame) {
      PRInt32 width, height;
      mFrame->GetWidth(&width);
      mFrame->GetHeight(&height);

      if ((width == (PRInt32)mInfo.image_width) &&
          (height == (PRInt32)mInfo.image_height)) {
        createNewFrame = PR_FALSE;
      } else {
        mImage->Clear();
      }
    }

    if (createNewFrame) {
      mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
      if (!mFrame) {
        mState = JPEG_ERROR;
        return NS_ERROR_OUT_OF_MEMORY;
      }

      gfx_format format = gfxIFormats::RGB;
#if defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS)
      format = gfxIFormats::BGR;
#endif

      if (NS_FAILED(mFrame->Init(0, 0, mInfo.image_width, mInfo.image_height, format, 24))) {
        mState = JPEG_ERROR;
        return NS_ERROR_OUT_OF_MEMORY;
      }

      mImage->AppendFrame(mFrame);
    }      

    mObserver->OnStartFrame(nsnull, mFrame);

    






    



    mSamples = (*mInfo.mem->alloc_sarray)((j_common_ptr) &mInfo,
                                          JPOOL_IMAGE,
                                          mInfo.output_width * 3, 1);

    mState = JPEG_START_DECOMPRESS;
  }

  case JPEG_START_DECOMPRESS:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::WriteFrom -- entering JPEG_START_DECOMPRESS case");
    

    


    mInfo.dct_method =  JDCT_ISLOW;
    mInfo.dither_mode = JDITHER_FS;
    mInfo.do_fancy_upsampling = TRUE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    
    if (jpeg_start_decompress(&mInfo) == FALSE)
      return NS_OK; 

    
    if (mInfo.buffered_image) {
      mState = JPEG_DECOMPRESS_PROGRESSIVE;
    } else {
      mState = JPEG_DECOMPRESS_SEQUENTIAL;
    }
  }

  case JPEG_DECOMPRESS_SEQUENTIAL:
  {
    if (mState == JPEG_DECOMPRESS_SEQUENTIAL)
    {
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::WriteFrom -- JPEG_DECOMPRESS_SEQUENTIAL case");
      
      if (!OutputScanlines())
        return NS_OK; 
      
      
      NS_ASSERTION(mInfo.output_scanline == mInfo.output_height, "We didn't process all of the data!");
      mState = JPEG_DONE;
    }
  }

  case JPEG_DECOMPRESS_PROGRESSIVE:
  {
    if (mState == JPEG_DECOMPRESS_PROGRESSIVE)
    {
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::WriteFrom -- JPEG_DECOMPRESS_PROGRESSIVE case");

      int status;
      do {
        status = jpeg_consume_input(&mInfo);
      } while ((status != JPEG_SUSPENDED) &&
               (status != JPEG_REACHED_EOI));

      for (;;) {
        if (mInfo.output_scanline == 0) {
          int scan = mInfo.input_scan_number;

          


          if ((mInfo.output_scan_number == 0) &&
              (scan > 1) &&
              (status != JPEG_REACHED_EOI))
            scan--;

          if (!jpeg_start_output(&mInfo, scan))
            return NS_OK; 
        }

        if (mInfo.output_scanline == 0xffffff)
          mInfo.output_scanline = 0;

        if (!OutputScanlines()) {
          if (mInfo.output_scanline == 0) {
            

            mInfo.output_scanline = 0xffffff;
          }
          return NS_OK; 
        }

        if (mInfo.output_scanline == mInfo.output_height)
        {
          if (!jpeg_finish_output(&mInfo))
            return NS_OK; 

          if (jpeg_input_complete(&mInfo) &&
              (mInfo.input_scan_number == mInfo.output_scan_number))
            break;

          mInfo.output_scanline = 0;
        }
      }

      mState = JPEG_DONE;
    }
  }

  case JPEG_DONE:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::WriteFrom -- entering JPEG_DONE case");

    

    if (jpeg_finish_decompress(&mInfo) == FALSE)
      return NS_OK; 

    mState = JPEG_SINK_NON_JPEG_TRAILER;

    
    break;
  }
  case JPEG_SINK_NON_JPEG_TRAILER:
    PR_LOG(gJPEGlog, PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::WriteFrom -- entering JPEG_SINK_NON_JPEG_TRAILER case\n", this));

    break;

  case JPEG_ERROR:
    PR_LOG(gJPEGlog, PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::WriteFrom -- entering JPEG_ERROR case\n", this));

    break;
  }

  return NS_OK;
}


PRBool
nsJPEGDecoder::OutputScanlines()
{
  const PRUint32 top = mInfo.output_scanline;
  PRBool rv = PR_TRUE;

  
  PRUint8 *imageData;
  PRUint32 imageDataLength;
  mFrame->GetImageData(&imageData, &imageDataLength);

  while ((mInfo.output_scanline < mInfo.output_height)) {
          
      if (jpeg_read_scanlines(&mInfo, mSamples, 1) != 1) {
        rv = PR_FALSE; 
        break;
      }

      if (mTransform) {
        if (mInfo.out_color_space == JCS_GRAYSCALE) {
          

          memcpy(mSamples[0] + 2 * mInfo.output_width,
                 mSamples[0],
                 mInfo.output_width);
          cmsDoTransform(mTransform,
                         mSamples[0] + 2 * mInfo.output_width, mSamples[0],
                         mInfo.output_width);
        } else
          cmsDoTransform(mTransform,
                         mSamples[0], mSamples[0],
                         mInfo.output_width);
      } else if (gfxPlatform::IsCMSEnabled()) {
        
        cmsHTRANSFORM transform = gfxPlatform::GetCMSRGBTransform();
        if (transform) {
          cmsDoTransform(transform,
                         mSamples[0], mSamples[0],
                         mInfo.output_width);
        }
      }

      
      PRUint32 offset = (mInfo.output_scanline - 1) * mInfo.output_width;
      PRUint32 *ptrOutputBuf = ((PRUint32*)imageData) + offset;
      JSAMPLE *j1 = mSamples[0];
      for (PRUint32 i=mInfo.output_width; i>0; --i) {
        *ptrOutputBuf++ = GFX_PACKED_PIXEL(0xFF, j1[0], j1[1], j1[2]);
        j1+=3;
      }
  }

  if (top != mInfo.output_scanline) {
      nsIntRect r(0, top, mInfo.output_width, mInfo.output_scanline-top);
      nsCOMPtr<nsIImage> img(do_GetInterface(mFrame));
      img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);
      mObserver->OnDataAvailable(nsnull, mFrame, &r);
  }

  return rv;
}



METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  decoder_error_mgr *err = (decoder_error_mgr *) cinfo->err;

  
  nsresult error_code = err->pub.msg_code == JERR_OUT_OF_MEMORY
                      ? NS_ERROR_OUT_OF_MEMORY
                      : NS_ERROR_FAILURE;

#ifdef DEBUG
  char buffer[JMSG_LENGTH_MAX];

  
  (*err->pub.format_message) (cinfo, buffer);

  fprintf(stderr, "JPEG decoding error:\n%s\n", buffer);
#endif

  
  longjmp(err->setjmp_buffer, error_code);
}











































METHODDEF(void)
init_source (j_decompress_ptr jd)
{
}












METHODDEF(void)
skip_input_data (j_decompress_ptr jd, long num_bytes)
{
  struct jpeg_source_mgr *src = jd->src;
  nsJPEGDecoder *decoder = (nsJPEGDecoder *)(jd->client_data);

  if (num_bytes > (long)src->bytes_in_buffer) {
    




    decoder->mBytesToSkip = (size_t)num_bytes - src->bytes_in_buffer;
    src->next_input_byte += src->bytes_in_buffer;
    src->bytes_in_buffer = 0;

  } else {
    

    src->bytes_in_buffer -= (size_t)num_bytes;
    src->next_input_byte += num_bytes;
  }
}














METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr jd)
{
  struct jpeg_source_mgr *src = jd->src;
  nsJPEGDecoder *decoder = (nsJPEGDecoder *)(jd->client_data);

  if (decoder->mReading) {
    unsigned char *new_buffer = (unsigned char *)decoder->mBuffer;
    PRUint32 new_buflen = decoder->mBufferLen;
  
    if (!new_buffer || new_buflen == 0)
      return PR_FALSE; 

    decoder->mBufferLen = 0;

    if (decoder->mBytesToSkip) {
      if (decoder->mBytesToSkip < new_buflen) {
        
        new_buffer += decoder->mBytesToSkip;
        new_buflen -= decoder->mBytesToSkip;
        decoder->mBytesToSkip = 0;
      } else {
        
        decoder->mBytesToSkip -= (size_t)new_buflen;
        return PR_FALSE; 
      }
    }

      decoder->mBackBufferUnreadLen = src->bytes_in_buffer;

    src->next_input_byte = new_buffer;
    src->bytes_in_buffer = (size_t)new_buflen;
    decoder->mReading = PR_FALSE;

    return PR_TRUE;
  }

  if (src->next_input_byte != decoder->mBuffer) {
    
    decoder->mBackBufferUnreadLen = 0;
    decoder->mBackBufferLen = 0;
  }

  
  const PRUint32 new_backtrack_buflen = src->bytes_in_buffer + decoder->mBackBufferLen;
 
  
  if (decoder->mBackBufferSize < new_backtrack_buflen) {


    
    const PRUint32 roundup_buflen = ((new_backtrack_buflen + 255) >> 8) << 8;

    if (decoder->mBackBuffer) {
      JOCTET *buf = (JOCTET *)PR_REALLOC(decoder->mBackBuffer, roundup_buflen);
      
      if (!buf) {
        decoder->mInfo.err->msg_code = JERR_OUT_OF_MEMORY;
        my_error_exit((j_common_ptr)(&decoder->mInfo));
      }
      decoder->mBackBuffer = buf;
    } else {
      decoder->mBackBuffer = (JOCTET*)PR_MALLOC(roundup_buflen);
      
      if (!decoder->mBackBuffer) {
        decoder->mInfo.err->msg_code = JERR_OUT_OF_MEMORY;
        my_error_exit((j_common_ptr)(&decoder->mInfo));
      }
    }
      
    decoder->mBackBufferSize = (size_t)roundup_buflen;

    
    if (new_backtrack_buflen > MAX_JPEG_MARKER_LENGTH) {
      my_error_exit((j_common_ptr)(&decoder->mInfo));
    }
  }

  
  memmove(decoder->mBackBuffer + decoder->mBackBufferLen,
          src->next_input_byte,
          src->bytes_in_buffer);

  
  src->next_input_byte = decoder->mBackBuffer + decoder->mBackBufferLen - decoder->mBackBufferUnreadLen;
  src->bytes_in_buffer += decoder->mBackBufferUnreadLen;
  decoder->mBackBufferLen = (size_t)new_backtrack_buflen;
  decoder->mReading = PR_TRUE;

  return PR_FALSE;
}








METHODDEF(void)
term_source (j_decompress_ptr jd)
{
  nsJPEGDecoder *decoder = (nsJPEGDecoder *)(jd->client_data);

  if (decoder->mObserver) {
    decoder->mObserver->OnStopFrame(nsnull, decoder->mFrame);
    decoder->mObserver->OnStopContainer(nsnull, decoder->mImage);
    decoder->mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }

  PRBool isMutable = PR_FALSE;
  if (decoder->mImageLoad) 
      decoder->mImageLoad->GetIsMultiPartChannel(&isMutable);
  decoder->mFrame->SetMutable(isMutable);
}
