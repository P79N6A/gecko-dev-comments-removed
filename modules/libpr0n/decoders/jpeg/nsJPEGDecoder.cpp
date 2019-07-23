








































#include "nsJPEGDecoder.h"

#include "imgIContainerObserver.h"

#include "nsIComponentManager.h"
#include "nsIInputStream.h"

#include "nspr.h"
#include "nsCRT.h"
#include "ImageLogging.h"
#include "nsIInterfaceRequestorUtils.h"
#include "gfxColor.h"

#include "jerror.h"

#include "gfxPlatform.h"

extern "C" {
#include "iccjpeg.h"


struct jpeg_color_deconverter {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, color_convert, (j_decompress_ptr cinfo,
				JSAMPIMAGE input_buf, JDIMENSION input_row,
				JSAMPARRAY output_buf, int num_rows));
};

METHODDEF(void)
ycc_rgb_convert_argb (j_decompress_ptr cinfo,
                 JSAMPIMAGE input_buf, JDIMENSION input_row,
                 JSAMPARRAY output_buf, int num_rows);
}

NS_IMPL_ISUPPORTS1(nsJPEGDecoder, imgIDecoder)

#if defined(PR_LOGGING)
PRLogModuleInfo *gJPEGlog = PR_NewLogModule("JPEGDecoder");
static PRLogModuleInfo *gJPEGDecoderAccountingLog = PR_NewLogModule("JPEGDecoderAccounting");
#else
#define gJPEGlog
#define gJPEGDecoderAccountingLog
#endif


METHODDEF(void) init_source (j_decompress_ptr jd);
METHODDEF(boolean) fill_input_buffer (j_decompress_ptr jd);
METHODDEF(void) skip_input_data (j_decompress_ptr jd, long num_bytes);
METHODDEF(void) term_source (j_decompress_ptr jd);
METHODDEF(void) my_error_exit (j_common_ptr cinfo);

static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width);


#define MAX_JPEG_MARKER_LENGTH  (((PRUint32)1 << 16) - 1)


nsJPEGDecoder::nsJPEGDecoder()
{
  mState = JPEG_HEADER;
  mReading = PR_TRUE;
  mNotifiedDone = PR_FALSE;
  mError = PR_FALSE;
  mImageData = nsnull;

  mBytesToSkip = 0;
  memset(&mInfo, 0, sizeof(jpeg_decompress_struct));
  memset(&mSourceMgr, 0, sizeof(mSourceMgr));
  mInfo.client_data = (void*)this;

  mSegment = nsnull;
  mSegmentLen = 0;

  mBackBuffer = nsnull;
  mBackBufferLen = mBackBufferSize = mBackBufferUnreadLen = 0;

  mInProfile = nsnull;
  mTransform = nsnull;

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("nsJPEGDecoder::nsJPEGDecoder: Creating JPEG decoder %p",
          this));
}

nsJPEGDecoder::~nsJPEGDecoder()
{
  PR_FREEIF(mBackBuffer);
  if (mTransform)
    qcms_transform_release(mTransform);
  if (mInProfile)
    qcms_profile_release(mInProfile);

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("nsJPEGDecoder::~nsJPEGDecoder: Destroying JPEG decoder %p",
          this));
}







NS_IMETHODIMP nsJPEGDecoder::Init(imgIContainer *aImage, 
                                  imgIDecoderObserver *aObserver,
                                  PRUint32 aFlags)
{

  
  mImage = aImage;
  mObserver = aObserver;
  mFlags = aFlags;

  
  if (!(mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) && mObserver)
    mObserver->OnStartDecode(nsnull);

  
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



NS_IMETHODIMP nsJPEGDecoder::Close(PRUint32 aFlags)
{
  PR_LOG(gJPEGlog, PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::Close\n", this));

  
  mInfo.src = nsnull;

  jpeg_destroy_decompress(&mInfo);

  

  if (mError)
    return NS_OK;

  

  if (!(aFlags & CLOSE_FLAG_DONTNOTIFY) &&
      !(mFlags && imgIDecoder::DECODER_FLAG_HEADERONLY) &&
      !mNotifiedDone)
    NotifyDone( PR_FALSE);

  
  return NS_OK;
}


NS_IMETHODIMP nsJPEGDecoder::Flush()
{
  LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Flush");

  if (mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER && mState != JPEG_ERROR)
    return this->ProcessData(nsnull, 0);

  return NS_OK;
}

static NS_METHOD ReadDataOut(nsIInputStream* in,
                             void* closure,
                             const char* fromRawSegment,
                             PRUint32 toOffset,
                             PRUint32 count,
                             PRUint32 *writeCount)
{
  nsJPEGDecoder *decoder = static_cast<nsJPEGDecoder*>(closure);

  
  *writeCount = count;

  
  nsresult rv = decoder->ProcessData(fromRawSegment, count);

  
  if (NS_FAILED(rv))
    decoder->mError = rv;
  return rv;
}



NS_IMETHODIMP nsJPEGDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count)
{
  NS_ENSURE_ARG_POINTER(inStr);

  
  nsresult rv = NS_OK;
  PRUint32 ignored;
  if (!mError)
    rv = inStr->ReadSegments(ReadDataOut, this, count, &ignored);
  if (mError || NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  return NS_OK;
}


nsresult nsJPEGDecoder::ProcessData(const char *data, PRUint32 count)
{
  LOG_SCOPE_WITH_PARAM(gJPEGlog, "nsJPEGDecoder::ProcessData", "count", count);

  mSegment = (const JOCTET *)data;
  mSegmentLen = count;

  
  nsresult error_code;
  if ((error_code = setjmp(mErr.setjmp_buffer)) != 0) {
    mState = JPEG_SINK_NON_JPEG_TRAILER;
    if (error_code == NS_ERROR_FAILURE) {
      

      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (setjmp returned NS_ERROR_FAILURE)"));
      return NS_OK;
    } else {
      


      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (setjmp returned an error)"));
      return error_code;
    }
  }

  PR_LOG(gJPEGlog, PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::ProcessData -- processing JPEG data\n", this));

  switch (mState) {
  case JPEG_HEADER:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::ProcessData -- entering JPEG_HEADER case");

    
    if (jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED) {
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (JPEG_SUSPENDED)"));
      return NS_OK; 
    }

    
    mImage->SetSize(mInfo.image_width, mInfo.image_height);
    if (mObserver)
      mObserver->OnStartContainer(nsnull, mImage);

    
    if (mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY)
      return NS_OK;

    
    JOCTET  *profile;
    PRUint32 profileLength;
    eCMSMode cmsMode = gfxPlatform::GetCMSMode();

    if ((cmsMode != eCMSMode_Off) &&
        read_icc_profile(&mInfo, &profile, &profileLength) &&
        (mInProfile = qcms_profile_from_memory(profile, profileLength)) != NULL) {
      free(profile);

      PRUint32 profileSpace = qcms_profile_get_color_space(mInProfile);
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
        else
	  
          mismatch = PR_TRUE;
        break;
      case JCS_CMYK:
      case JCS_YCCK:
	  
          mismatch = PR_TRUE;
        break;
      default:
        mState = JPEG_ERROR;
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (unknown colorpsace (1))"));
        return NS_ERROR_UNEXPECTED;
      }

      if (!mismatch) {
        qcms_data_type type;
        switch (mInfo.out_color_space) {
        case JCS_GRAYSCALE:
          type = QCMS_DATA_GRAY_8;
          break;
        case JCS_RGB:
          type = QCMS_DATA_RGB_8;
          break;
        default:
          mState = JPEG_ERROR;
          PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                 ("} (unknown colorpsace (2))"));
          return NS_ERROR_UNEXPECTED;
        }
#if 0
        



        
        if (mInfo.out_color_space == JCS_CMYK)
          type |= FLAVOR_SH(mInfo.saw_Adobe_marker ? 1 : 0);
#endif

        if (gfxPlatform::GetCMSOutputProfile()) {

          
          int intent = gfxPlatform::GetRenderingIntent();
          if (intent == -1)
              intent = qcms_profile_get_rendering_intent(mInProfile);

          
          mTransform = qcms_transform_create(mInProfile,
                                          type,
                                          gfxPlatform::GetCMSOutputProfile(),
                                          QCMS_DATA_RGB_8,
                                          (qcms_intent)intent);
        }
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
      case JCS_CMYK:
      case JCS_YCCK:
        
        mInfo.out_color_space = JCS_CMYK;
        break;
      default:
        mState = JPEG_ERROR;
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (unknown colorpsace (3))"));
        return NS_ERROR_UNEXPECTED;
        break;
      }
    }

    



    mInfo.buffered_image = jpeg_has_multiple_scans(&mInfo);

    
    jpeg_calc_output_dimensions(&mInfo);


    
    
    PRUint32 imagelength;
    if (NS_FAILED(mImage->EnsureCleanFrame(0, 0, 0, mInfo.image_width, mInfo.image_height,
                                           gfxASurface::ImageFormatRGB24,
                                           &mImageData, &imagelength))) {
      mState = JPEG_ERROR;
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (could not initialize image frame)"));
      return NS_ERROR_OUT_OF_MEMORY;
    }

    PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
           ("        JPEGDecoderAccounting: nsJPEGDecoder::ProcessData -- created image frame with %ux%u pixels",
            mInfo.image_width, mInfo.image_height));

    if (mObserver)
      mObserver->OnStartFrame(nsnull, 0);
    mState = JPEG_START_DECOMPRESS;
  }

  case JPEG_START_DECOMPRESS:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::ProcessData -- entering JPEG_START_DECOMPRESS case");
    

    


    mInfo.dct_method =  JDCT_ISLOW;
    mInfo.dither_mode = JDITHER_FS;
    mInfo.do_fancy_upsampling = TRUE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    
    if (jpeg_start_decompress(&mInfo) == FALSE) {
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (I/O suspension after jpeg_start_decompress())"));
      return NS_OK; 
    }

    
    if (!mTransform && (gfxPlatform::GetCMSMode() == eCMSMode_Off) &&
        mInfo.jpeg_color_space == JCS_YCbCr && mInfo.out_color_space == JCS_RGB) {
      
      mInfo.out_color_components = 4; 
      mInfo.cconvert->color_convert = ycc_rgb_convert_argb;
    }

    
    mState = mInfo.buffered_image ? JPEG_DECOMPRESS_PROGRESSIVE : JPEG_DECOMPRESS_SEQUENTIAL;
  }

  case JPEG_DECOMPRESS_SEQUENTIAL:
  {
    if (mState == JPEG_DECOMPRESS_SEQUENTIAL)
    {
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::ProcessData -- JPEG_DECOMPRESS_SEQUENTIAL case");
      
      PRBool suspend;
      nsresult rv = OutputScanlines(&suspend);
      if (NS_FAILED(rv))
        return rv;
      
      if (suspend) {
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (I/O suspension after OutputScanlines() - SEQUENTIAL)"));
        return NS_OK; 
      }
      
      
      NS_ASSERTION(mInfo.output_scanline == mInfo.output_height, "We didn't process all of the data!");
      mState = JPEG_DONE;
    }
  }

  case JPEG_DECOMPRESS_PROGRESSIVE:
  {
    if (mState == JPEG_DECOMPRESS_PROGRESSIVE)
    {
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::ProcessData -- JPEG_DECOMPRESS_PROGRESSIVE case");

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

          if (!jpeg_start_output(&mInfo, scan)) {
            PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                   ("} (I/O suspension after jpeg_start_output() - PROGRESSIVE)"));
            return NS_OK; 
          }
        }

        if (mInfo.output_scanline == 0xffffff)
          mInfo.output_scanline = 0;

        PRBool suspend;
        nsresult rv = OutputScanlines(&suspend);
        if (NS_FAILED(rv))
          return rv;

        if (suspend) {
          if (mInfo.output_scanline == 0) {
            

            mInfo.output_scanline = 0xffffff;
          }
          PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                 ("} (I/O suspension after OutputScanlines() - PROGRESSIVE)"));
          return NS_OK; 
        }

        if (mInfo.output_scanline == mInfo.output_height)
        {
          if (!jpeg_finish_output(&mInfo)) {
            PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                   ("} (I/O suspension after jpeg_finish_output() - PROGRESSIVE)"));
            return NS_OK; 
          }

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
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::ProcessData -- entering JPEG_DONE case");

    

    if (jpeg_finish_decompress(&mInfo) == FALSE) {
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (I/O suspension after jpeg_finish_decompress() - DONE)"));
      return NS_OK; 
    }

    mState = JPEG_SINK_NON_JPEG_TRAILER;

    
    break;
  }
  case JPEG_SINK_NON_JPEG_TRAILER:
    PR_LOG(gJPEGlog, PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::ProcessData -- entering JPEG_SINK_NON_JPEG_TRAILER case\n", this));

    break;

  case JPEG_ERROR:
    PR_LOG(gJPEGlog, PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::ProcessData -- entering JPEG_ERROR case\n", this));

    break;
  }

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("} (end of function)"));
  return NS_OK;
}

void
nsJPEGDecoder::NotifyDone(PRBool aSuccess)
{
  
  NS_ABORT_IF_FALSE(!mNotifiedDone, "calling NotifyDone twice!");

  
  if (mObserver)
    mObserver->OnStopFrame(nsnull, 0);
  if (aSuccess)
    mImage->DecodingComplete();
  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, aSuccess ? NS_OK : NS_ERROR_FAILURE,
                            nsnull);
  }

  
  mNotifiedDone = PR_TRUE;
}

nsresult
nsJPEGDecoder::OutputScanlines(PRBool* suspend)
{
  *suspend = PR_FALSE;

  const PRUint32 top = mInfo.output_scanline;
  nsresult rv = NS_OK;

  while ((mInfo.output_scanline < mInfo.output_height)) {
      
      PRUint32 *imageRow = ((PRUint32*)mImageData) +
                           (mInfo.output_scanline * mInfo.output_width);

      if (mInfo.cconvert->color_convert == ycc_rgb_convert_argb) {
        
        if (jpeg_read_scanlines(&mInfo, (JSAMPARRAY)&imageRow, 1) != 1) {
          *suspend = PR_TRUE; 
          break;
        }
        continue; 
      }

      JSAMPROW sampleRow = (JSAMPROW)imageRow;
      if (mInfo.output_components == 3) {
        
        sampleRow += mInfo.output_width;
      }

          
      if (jpeg_read_scanlines(&mInfo, &sampleRow, 1) != 1) {
        *suspend = PR_TRUE; 
        break;
      }

      if (mTransform) {
        JSAMPROW source = sampleRow;
        if (mInfo.out_color_space == JCS_GRAYSCALE) {
          

          sampleRow += mInfo.output_width;
        }
        qcms_transform_data(mTransform, source, sampleRow, mInfo.output_width);
        
        if (mInfo.out_color_space == JCS_CMYK) {
          memmove(sampleRow + mInfo.output_width,
                  sampleRow,
                  3 * mInfo.output_width);
          sampleRow += mInfo.output_width;
        }
      } else {
        if (mInfo.out_color_space == JCS_CMYK) {
          
          
          
          cmyk_convert_rgb((JSAMPROW)imageRow, mInfo.output_width);
          sampleRow += mInfo.output_width;
        }
        if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
          
          qcms_transform *transform = gfxPlatform::GetCMSRGBTransform();
          if (transform) {
            qcms_transform_data(transform, sampleRow, sampleRow, mInfo.output_width);
          }
        }
      }

      
      PRUint32 idx = mInfo.output_width;

      
      for (; (NS_PTR_TO_UINT32(sampleRow) & 0x3) && idx; --idx) {
        *imageRow++ = GFX_PACKED_PIXEL(0xFF, sampleRow[0], sampleRow[1], sampleRow[2]);
        sampleRow += 3;
      }

      
      while (idx >= 4) {
        GFX_BLOCK_RGB_TO_FRGB(sampleRow, imageRow);
        idx       -=  4;
        sampleRow += 12;
        imageRow  +=  4;
      }

      
      while (idx--) {
        
        *imageRow++ = GFX_PACKED_PIXEL(0xFF, sampleRow[0], sampleRow[1], sampleRow[2]);
        sampleRow += 3;
      }
  }

  if (top != mInfo.output_scanline) {
      nsIntRect r(0, top, mInfo.output_width, mInfo.output_scanline-top);
      rv = mImage->FrameUpdated(0, r);
      if (mObserver)
        mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);
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
    const JOCTET *new_buffer = decoder->mSegment;
    PRUint32 new_buflen = decoder->mSegmentLen;
  
    if (!new_buffer || new_buflen == 0)
      return PR_FALSE; 

    decoder->mSegmentLen = 0;

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

  if (src->next_input_byte != decoder->mSegment) {
    
    decoder->mBackBufferUnreadLen = 0;
    decoder->mBackBufferLen = 0;
  }

  
  const PRUint32 new_backtrack_buflen = src->bytes_in_buffer + decoder->mBackBufferLen;
 
  
  if (decoder->mBackBufferSize < new_backtrack_buflen) {
    
    if (new_backtrack_buflen > MAX_JPEG_MARKER_LENGTH) {
      my_error_exit((j_common_ptr)(&decoder->mInfo));
    }

    
    const size_t roundup_buflen = ((new_backtrack_buflen + 255) >> 8) << 8;
    JOCTET *buf = (JOCTET *)PR_REALLOC(decoder->mBackBuffer, roundup_buflen);
    
    if (!buf) {
      decoder->mInfo.err->msg_code = JERR_OUT_OF_MEMORY;
      my_error_exit((j_common_ptr)(&decoder->mInfo));
    }
    decoder->mBackBuffer = buf;
    decoder->mBackBufferSize = roundup_buflen;
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

  
  NS_ABORT_IF_FALSE(!decoder->mError,
                    "Calling term_source on a JPEG with mError=true!");

  
  decoder->NotifyDone( PR_TRUE);
}































#define SCALEBITS       16      /* speediest right-shift on some machines */




const int Cr_r_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
  0xffffff4dUL, 0xffffff4eUL, 0xffffff4fUL, 0xffffff51UL, 0xffffff52UL, 0xffffff54UL, 
  0xffffff55UL, 0xffffff56UL, 0xffffff58UL, 0xffffff59UL, 0xffffff5bUL, 0xffffff5cUL, 
  0xffffff5dUL, 0xffffff5fUL, 0xffffff60UL, 0xffffff62UL, 0xffffff63UL, 0xffffff64UL, 
  0xffffff66UL, 0xffffff67UL, 0xffffff69UL, 0xffffff6aUL, 0xffffff6bUL, 0xffffff6dUL, 
  0xffffff6eUL, 0xffffff70UL, 0xffffff71UL, 0xffffff72UL, 0xffffff74UL, 0xffffff75UL, 
  0xffffff77UL, 0xffffff78UL, 0xffffff79UL, 0xffffff7bUL, 0xffffff7cUL, 0xffffff7eUL, 
  0xffffff7fUL, 0xffffff80UL, 0xffffff82UL, 0xffffff83UL, 0xffffff85UL, 0xffffff86UL, 
  0xffffff87UL, 0xffffff89UL, 0xffffff8aUL, 0xffffff8cUL, 0xffffff8dUL, 0xffffff8eUL, 
  0xffffff90UL, 0xffffff91UL, 0xffffff93UL, 0xffffff94UL, 0xffffff95UL, 0xffffff97UL, 
  0xffffff98UL, 0xffffff9aUL, 0xffffff9bUL, 0xffffff9cUL, 0xffffff9eUL, 0xffffff9fUL, 
  0xffffffa1UL, 0xffffffa2UL, 0xffffffa3UL, 0xffffffa5UL, 0xffffffa6UL, 0xffffffa8UL, 
  0xffffffa9UL, 0xffffffaaUL, 0xffffffacUL, 0xffffffadUL, 0xffffffafUL, 0xffffffb0UL, 
  0xffffffb1UL, 0xffffffb3UL, 0xffffffb4UL, 0xffffffb6UL, 0xffffffb7UL, 0xffffffb8UL, 
  0xffffffbaUL, 0xffffffbbUL, 0xffffffbdUL, 0xffffffbeUL, 0xffffffc0UL, 0xffffffc1UL, 
  0xffffffc2UL, 0xffffffc4UL, 0xffffffc5UL, 0xffffffc7UL, 0xffffffc8UL, 0xffffffc9UL, 
  0xffffffcbUL, 0xffffffccUL, 0xffffffceUL, 0xffffffcfUL, 0xffffffd0UL, 0xffffffd2UL, 
  0xffffffd3UL, 0xffffffd5UL, 0xffffffd6UL, 0xffffffd7UL, 0xffffffd9UL, 0xffffffdaUL, 
  0xffffffdcUL, 0xffffffddUL, 0xffffffdeUL, 0xffffffe0UL, 0xffffffe1UL, 0xffffffe3UL, 
  0xffffffe4UL, 0xffffffe5UL, 0xffffffe7UL, 0xffffffe8UL, 0xffffffeaUL, 0xffffffebUL, 
  0xffffffecUL, 0xffffffeeUL, 0xffffffefUL, 0xfffffff1UL, 0xfffffff2UL, 0xfffffff3UL, 
  0xfffffff5UL, 0xfffffff6UL, 0xfffffff8UL, 0xfffffff9UL, 0xfffffffaUL, 0xfffffffcUL, 
  0xfffffffdUL, 0xffffffffUL,       0x00UL,       0x01UL,       0x03UL,       0x04UL, 
        0x06UL,       0x07UL,       0x08UL,       0x0aUL,       0x0bUL,       0x0dUL, 
        0x0eUL,       0x0fUL,       0x11UL,       0x12UL,       0x14UL,       0x15UL, 
        0x16UL,       0x18UL,       0x19UL,       0x1bUL,       0x1cUL,       0x1dUL, 
        0x1fUL,       0x20UL,       0x22UL,       0x23UL,       0x24UL,       0x26UL, 
        0x27UL,       0x29UL,       0x2aUL,       0x2bUL,       0x2dUL,       0x2eUL, 
        0x30UL,       0x31UL,       0x32UL,       0x34UL,       0x35UL,       0x37UL, 
        0x38UL,       0x39UL,       0x3bUL,       0x3cUL,       0x3eUL,       0x3fUL, 
        0x40UL,       0x42UL,       0x43UL,       0x45UL,       0x46UL,       0x48UL, 
        0x49UL,       0x4aUL,       0x4cUL,       0x4dUL,       0x4fUL,       0x50UL, 
        0x51UL,       0x53UL,       0x54UL,       0x56UL,       0x57UL,       0x58UL, 
        0x5aUL,       0x5bUL,       0x5dUL,       0x5eUL,       0x5fUL,       0x61UL, 
        0x62UL,       0x64UL,       0x65UL,       0x66UL,       0x68UL,       0x69UL, 
        0x6bUL,       0x6cUL,       0x6dUL,       0x6fUL,       0x70UL,       0x72UL, 
        0x73UL,       0x74UL,       0x76UL,       0x77UL,       0x79UL,       0x7aUL, 
        0x7bUL,       0x7dUL,       0x7eUL,       0x80UL,       0x81UL,       0x82UL, 
        0x84UL,       0x85UL,       0x87UL,       0x88UL,       0x89UL,       0x8bUL, 
        0x8cUL,       0x8eUL,       0x8fUL,       0x90UL,       0x92UL,       0x93UL, 
        0x95UL,       0x96UL,       0x97UL,       0x99UL,       0x9aUL,       0x9cUL, 
        0x9dUL,       0x9eUL,       0xa0UL,       0xa1UL,       0xa3UL,       0xa4UL, 
        0xa5UL,       0xa7UL,       0xa8UL,       0xaaUL,       0xabUL,       0xacUL, 
        0xaeUL,       0xafUL,       0xb1UL,       0xb2UL
  };

const int Cb_b_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
  0xffffff1dUL, 0xffffff1fUL, 0xffffff21UL, 0xffffff22UL, 0xffffff24UL, 0xffffff26UL, 
  0xffffff28UL, 0xffffff2aUL, 0xffffff2bUL, 0xffffff2dUL, 0xffffff2fUL, 0xffffff31UL, 
  0xffffff32UL, 0xffffff34UL, 0xffffff36UL, 0xffffff38UL, 0xffffff3aUL, 0xffffff3bUL, 
  0xffffff3dUL, 0xffffff3fUL, 0xffffff41UL, 0xffffff42UL, 0xffffff44UL, 0xffffff46UL, 
  0xffffff48UL, 0xffffff49UL, 0xffffff4bUL, 0xffffff4dUL, 0xffffff4fUL, 0xffffff51UL, 
  0xffffff52UL, 0xffffff54UL, 0xffffff56UL, 0xffffff58UL, 0xffffff59UL, 0xffffff5bUL, 
  0xffffff5dUL, 0xffffff5fUL, 0xffffff61UL, 0xffffff62UL, 0xffffff64UL, 0xffffff66UL, 
  0xffffff68UL, 0xffffff69UL, 0xffffff6bUL, 0xffffff6dUL, 0xffffff6fUL, 0xffffff70UL, 
  0xffffff72UL, 0xffffff74UL, 0xffffff76UL, 0xffffff78UL, 0xffffff79UL, 0xffffff7bUL, 
  0xffffff7dUL, 0xffffff7fUL, 0xffffff80UL, 0xffffff82UL, 0xffffff84UL, 0xffffff86UL, 
  0xffffff88UL, 0xffffff89UL, 0xffffff8bUL, 0xffffff8dUL, 0xffffff8fUL, 0xffffff90UL, 
  0xffffff92UL, 0xffffff94UL, 0xffffff96UL, 0xffffff97UL, 0xffffff99UL, 0xffffff9bUL, 
  0xffffff9dUL, 0xffffff9fUL, 0xffffffa0UL, 0xffffffa2UL, 0xffffffa4UL, 0xffffffa6UL, 
  0xffffffa7UL, 0xffffffa9UL, 0xffffffabUL, 0xffffffadUL, 0xffffffaeUL, 0xffffffb0UL, 
  0xffffffb2UL, 0xffffffb4UL, 0xffffffb6UL, 0xffffffb7UL, 0xffffffb9UL, 0xffffffbbUL, 
  0xffffffbdUL, 0xffffffbeUL, 0xffffffc0UL, 0xffffffc2UL, 0xffffffc4UL, 0xffffffc6UL, 
  0xffffffc7UL, 0xffffffc9UL, 0xffffffcbUL, 0xffffffcdUL, 0xffffffceUL, 0xffffffd0UL, 
  0xffffffd2UL, 0xffffffd4UL, 0xffffffd5UL, 0xffffffd7UL, 0xffffffd9UL, 0xffffffdbUL, 
  0xffffffddUL, 0xffffffdeUL, 0xffffffe0UL, 0xffffffe2UL, 0xffffffe4UL, 0xffffffe5UL, 
  0xffffffe7UL, 0xffffffe9UL, 0xffffffebUL, 0xffffffedUL, 0xffffffeeUL, 0xfffffff0UL, 
  0xfffffff2UL, 0xfffffff4UL, 0xfffffff5UL, 0xfffffff7UL, 0xfffffff9UL, 0xfffffffbUL, 
  0xfffffffcUL, 0xfffffffeUL,       0x00UL,       0x02UL,       0x04UL,       0x05UL, 
        0x07UL,       0x09UL,       0x0bUL,       0x0cUL,       0x0eUL,       0x10UL, 
        0x12UL,       0x13UL,       0x15UL,       0x17UL,       0x19UL,       0x1bUL, 
        0x1cUL,       0x1eUL,       0x20UL,       0x22UL,       0x23UL,       0x25UL, 
        0x27UL,       0x29UL,       0x2bUL,       0x2cUL,       0x2eUL,       0x30UL, 
        0x32UL,       0x33UL,       0x35UL,       0x37UL,       0x39UL,       0x3aUL, 
        0x3cUL,       0x3eUL,       0x40UL,       0x42UL,       0x43UL,       0x45UL, 
        0x47UL,       0x49UL,       0x4aUL,       0x4cUL,       0x4eUL,       0x50UL, 
        0x52UL,       0x53UL,       0x55UL,       0x57UL,       0x59UL,       0x5aUL, 
        0x5cUL,       0x5eUL,       0x60UL,       0x61UL,       0x63UL,       0x65UL, 
        0x67UL,       0x69UL,       0x6aUL,       0x6cUL,       0x6eUL,       0x70UL, 
        0x71UL,       0x73UL,       0x75UL,       0x77UL,       0x78UL,       0x7aUL, 
        0x7cUL,       0x7eUL,       0x80UL,       0x81UL,       0x83UL,       0x85UL, 
        0x87UL,       0x88UL,       0x8aUL,       0x8cUL,       0x8eUL,       0x90UL, 
        0x91UL,       0x93UL,       0x95UL,       0x97UL,       0x98UL,       0x9aUL, 
        0x9cUL,       0x9eUL,       0x9fUL,       0xa1UL,       0xa3UL,       0xa5UL, 
        0xa7UL,       0xa8UL,       0xaaUL,       0xacUL,       0xaeUL,       0xafUL, 
        0xb1UL,       0xb3UL,       0xb5UL,       0xb7UL,       0xb8UL,       0xbaUL, 
        0xbcUL,       0xbeUL,       0xbfUL,       0xc1UL,       0xc3UL,       0xc5UL, 
        0xc6UL,       0xc8UL,       0xcaUL,       0xccUL,       0xceUL,       0xcfUL, 
        0xd1UL,       0xd3UL,       0xd5UL,       0xd6UL,       0xd8UL,       0xdaUL, 
        0xdcUL,       0xdeUL,       0xdfUL,       0xe1UL
  };

const int Cr_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x5b6900UL,   0x5ab22eUL,   0x59fb5cUL,   0x59448aUL,   0x588db8UL,   0x57d6e6UL, 
    0x572014UL,   0x566942UL,   0x55b270UL,   0x54fb9eUL,   0x5444ccUL,   0x538dfaUL, 
    0x52d728UL,   0x522056UL,   0x516984UL,   0x50b2b2UL,   0x4ffbe0UL,   0x4f450eUL, 
    0x4e8e3cUL,   0x4dd76aUL,   0x4d2098UL,   0x4c69c6UL,   0x4bb2f4UL,   0x4afc22UL, 
    0x4a4550UL,   0x498e7eUL,   0x48d7acUL,   0x4820daUL,   0x476a08UL,   0x46b336UL, 
    0x45fc64UL,   0x454592UL,   0x448ec0UL,   0x43d7eeUL,   0x43211cUL,   0x426a4aUL, 
    0x41b378UL,   0x40fca6UL,   0x4045d4UL,   0x3f8f02UL,   0x3ed830UL,   0x3e215eUL, 
    0x3d6a8cUL,   0x3cb3baUL,   0x3bfce8UL,   0x3b4616UL,   0x3a8f44UL,   0x39d872UL, 
    0x3921a0UL,   0x386aceUL,   0x37b3fcUL,   0x36fd2aUL,   0x364658UL,   0x358f86UL, 
    0x34d8b4UL,   0x3421e2UL,   0x336b10UL,   0x32b43eUL,   0x31fd6cUL,   0x31469aUL, 
    0x308fc8UL,   0x2fd8f6UL,   0x2f2224UL,   0x2e6b52UL,   0x2db480UL,   0x2cfdaeUL, 
    0x2c46dcUL,   0x2b900aUL,   0x2ad938UL,   0x2a2266UL,   0x296b94UL,   0x28b4c2UL, 
    0x27fdf0UL,   0x27471eUL,   0x26904cUL,   0x25d97aUL,   0x2522a8UL,   0x246bd6UL, 
    0x23b504UL,   0x22fe32UL,   0x224760UL,   0x21908eUL,   0x20d9bcUL,   0x2022eaUL, 
    0x1f6c18UL,   0x1eb546UL,   0x1dfe74UL,   0x1d47a2UL,   0x1c90d0UL,   0x1bd9feUL, 
    0x1b232cUL,   0x1a6c5aUL,   0x19b588UL,   0x18feb6UL,   0x1847e4UL,   0x179112UL, 
    0x16da40UL,   0x16236eUL,   0x156c9cUL,   0x14b5caUL,   0x13fef8UL,   0x134826UL, 
    0x129154UL,   0x11da82UL,   0x1123b0UL,   0x106cdeUL,    0xfb60cUL,    0xeff3aUL, 
     0xe4868UL,    0xd9196UL,    0xcdac4UL,    0xc23f2UL,    0xb6d20UL,    0xab64eUL, 
     0x9ff7cUL,    0x948aaUL,    0x891d8UL,    0x7db06UL,    0x72434UL,    0x66d62UL, 
     0x5b690UL,    0x4ffbeUL,    0x448ecUL,    0x3921aUL,    0x2db48UL,    0x22476UL, 
     0x16da4UL,     0xb6d2UL,        0x0UL, 0xffff492eUL, 0xfffe925cUL, 0xfffddb8aUL, 
  0xfffd24b8UL, 0xfffc6de6UL, 0xfffbb714UL, 0xfffb0042UL, 0xfffa4970UL, 0xfff9929eUL, 
  0xfff8dbccUL, 0xfff824faUL, 0xfff76e28UL, 0xfff6b756UL, 0xfff60084UL, 0xfff549b2UL, 
  0xfff492e0UL, 0xfff3dc0eUL, 0xfff3253cUL, 0xfff26e6aUL, 0xfff1b798UL, 0xfff100c6UL, 
  0xfff049f4UL, 0xffef9322UL, 0xffeedc50UL, 0xffee257eUL, 0xffed6eacUL, 0xffecb7daUL, 
  0xffec0108UL, 0xffeb4a36UL, 0xffea9364UL, 0xffe9dc92UL, 0xffe925c0UL, 0xffe86eeeUL, 
  0xffe7b81cUL, 0xffe7014aUL, 0xffe64a78UL, 0xffe593a6UL, 0xffe4dcd4UL, 0xffe42602UL, 
  0xffe36f30UL, 0xffe2b85eUL, 0xffe2018cUL, 0xffe14abaUL, 0xffe093e8UL, 0xffdfdd16UL, 
  0xffdf2644UL, 0xffde6f72UL, 0xffddb8a0UL, 0xffdd01ceUL, 0xffdc4afcUL, 0xffdb942aUL, 
  0xffdadd58UL, 0xffda2686UL, 0xffd96fb4UL, 0xffd8b8e2UL, 0xffd80210UL, 0xffd74b3eUL, 
  0xffd6946cUL, 0xffd5dd9aUL, 0xffd526c8UL, 0xffd46ff6UL, 0xffd3b924UL, 0xffd30252UL, 
  0xffd24b80UL, 0xffd194aeUL, 0xffd0dddcUL, 0xffd0270aUL, 0xffcf7038UL, 0xffceb966UL, 
  0xffce0294UL, 0xffcd4bc2UL, 0xffcc94f0UL, 0xffcbde1eUL, 0xffcb274cUL, 0xffca707aUL, 
  0xffc9b9a8UL, 0xffc902d6UL, 0xffc84c04UL, 0xffc79532UL, 0xffc6de60UL, 0xffc6278eUL, 
  0xffc570bcUL, 0xffc4b9eaUL, 0xffc40318UL, 0xffc34c46UL, 0xffc29574UL, 0xffc1dea2UL, 
  0xffc127d0UL, 0xffc070feUL, 0xffbfba2cUL, 0xffbf035aUL, 0xffbe4c88UL, 0xffbd95b6UL, 
  0xffbcdee4UL, 0xffbc2812UL, 0xffbb7140UL, 0xffbaba6eUL, 0xffba039cUL, 0xffb94ccaUL, 
  0xffb895f8UL, 0xffb7df26UL, 0xffb72854UL, 0xffb67182UL, 0xffb5bab0UL, 0xffb503deUL, 
  0xffb44d0cUL, 0xffb3963aUL, 0xffb2df68UL, 0xffb22896UL, 0xffb171c4UL, 0xffb0baf2UL, 
  0xffb00420UL, 0xffaf4d4eUL, 0xffae967cUL, 0xffaddfaaUL, 0xffad28d8UL, 0xffac7206UL, 
  0xffabbb34UL, 0xffab0462UL, 0xffaa4d90UL, 0xffa996beUL, 0xffa8dfecUL, 0xffa8291aUL, 
  0xffa77248UL, 0xffa6bb76UL, 0xffa604a4UL, 0xffa54dd2UL
 };

const int Cb_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x2c8d00UL,   0x2c34e6UL,   0x2bdcccUL,   0x2b84b2UL,   0x2b2c98UL,   0x2ad47eUL, 
    0x2a7c64UL,   0x2a244aUL,   0x29cc30UL,   0x297416UL,   0x291bfcUL,   0x28c3e2UL, 
    0x286bc8UL,   0x2813aeUL,   0x27bb94UL,   0x27637aUL,   0x270b60UL,   0x26b346UL, 
    0x265b2cUL,   0x260312UL,   0x25aaf8UL,   0x2552deUL,   0x24fac4UL,   0x24a2aaUL, 
    0x244a90UL,   0x23f276UL,   0x239a5cUL,   0x234242UL,   0x22ea28UL,   0x22920eUL, 
    0x2239f4UL,   0x21e1daUL,   0x2189c0UL,   0x2131a6UL,   0x20d98cUL,   0x208172UL, 
    0x202958UL,   0x1fd13eUL,   0x1f7924UL,   0x1f210aUL,   0x1ec8f0UL,   0x1e70d6UL, 
    0x1e18bcUL,   0x1dc0a2UL,   0x1d6888UL,   0x1d106eUL,   0x1cb854UL,   0x1c603aUL, 
    0x1c0820UL,   0x1bb006UL,   0x1b57ecUL,   0x1affd2UL,   0x1aa7b8UL,   0x1a4f9eUL, 
    0x19f784UL,   0x199f6aUL,   0x194750UL,   0x18ef36UL,   0x18971cUL,   0x183f02UL, 
    0x17e6e8UL,   0x178eceUL,   0x1736b4UL,   0x16de9aUL,   0x168680UL,   0x162e66UL, 
    0x15d64cUL,   0x157e32UL,   0x152618UL,   0x14cdfeUL,   0x1475e4UL,   0x141dcaUL, 
    0x13c5b0UL,   0x136d96UL,   0x13157cUL,   0x12bd62UL,   0x126548UL,   0x120d2eUL, 
    0x11b514UL,   0x115cfaUL,   0x1104e0UL,   0x10acc6UL,   0x1054acUL,    0xffc92UL, 
     0xfa478UL,    0xf4c5eUL,    0xef444UL,    0xe9c2aUL,    0xe4410UL,    0xdebf6UL, 
     0xd93dcUL,    0xd3bc2UL,    0xce3a8UL,    0xc8b8eUL,    0xc3374UL,    0xbdb5aUL, 
     0xb8340UL,    0xb2b26UL,    0xad30cUL,    0xa7af2UL,    0xa22d8UL,    0x9cabeUL, 
     0x972a4UL,    0x91a8aUL,    0x8c270UL,    0x86a56UL,    0x8123cUL,    0x7ba22UL, 
     0x76208UL,    0x709eeUL,    0x6b1d4UL,    0x659baUL,    0x601a0UL,    0x5a986UL, 
     0x5516cUL,    0x4f952UL,    0x4a138UL,    0x4491eUL,    0x3f104UL,    0x398eaUL, 
     0x340d0UL,    0x2e8b6UL,    0x2909cUL,    0x23882UL,    0x1e068UL,    0x1884eUL, 
     0x13034UL,     0xd81aUL,     0x8000UL,     0x27e6UL, 0xffffcfccUL, 0xffff77b2UL,
  0xffff1f98UL, 0xfffec77eUL, 0xfffe6f64UL, 0xfffe174aUL, 0xfffdbf30UL, 0xfffd6716UL,
  0xfffd0efcUL, 0xfffcb6e2UL, 0xfffc5ec8UL, 0xfffc06aeUL, 0xfffbae94UL, 0xfffb567aUL,
  0xfffafe60UL, 0xfffaa646UL, 0xfffa4e2cUL, 0xfff9f612UL, 0xfff99df8UL, 0xfff945deUL,
  0xfff8edc4UL, 0xfff895aaUL, 0xfff83d90UL, 0xfff7e576UL, 0xfff78d5cUL, 0xfff73542UL,
  0xfff6dd28UL, 0xfff6850eUL, 0xfff62cf4UL, 0xfff5d4daUL, 0xfff57cc0UL, 0xfff524a6UL,
  0xfff4cc8cUL, 0xfff47472UL, 0xfff41c58UL, 0xfff3c43eUL, 0xfff36c24UL, 0xfff3140aUL,
  0xfff2bbf0UL, 0xfff263d6UL, 0xfff20bbcUL, 0xfff1b3a2UL, 0xfff15b88UL, 0xfff1036eUL,
  0xfff0ab54UL, 0xfff0533aUL, 0xffeffb20UL, 0xffefa306UL, 0xffef4aecUL, 0xffeef2d2UL,
  0xffee9ab8UL, 0xffee429eUL, 0xffedea84UL, 0xffed926aUL, 0xffed3a50UL, 0xffece236UL,
  0xffec8a1cUL, 0xffec3202UL, 0xffebd9e8UL, 0xffeb81ceUL, 0xffeb29b4UL, 0xffead19aUL,
  0xffea7980UL, 0xffea2166UL, 0xffe9c94cUL, 0xffe97132UL, 0xffe91918UL, 0xffe8c0feUL,
  0xffe868e4UL, 0xffe810caUL, 0xffe7b8b0UL, 0xffe76096UL, 0xffe7087cUL, 0xffe6b062UL,
  0xffe65848UL, 0xffe6002eUL, 0xffe5a814UL, 0xffe54ffaUL, 0xffe4f7e0UL, 0xffe49fc6UL,
  0xffe447acUL, 0xffe3ef92UL, 0xffe39778UL, 0xffe33f5eUL, 0xffe2e744UL, 0xffe28f2aUL,
  0xffe23710UL, 0xffe1def6UL, 0xffe186dcUL, 0xffe12ec2UL, 0xffe0d6a8UL, 0xffe07e8eUL,
  0xffe02674UL, 0xffdfce5aUL, 0xffdf7640UL, 0xffdf1e26UL, 0xffdec60cUL, 0xffde6df2UL,
  0xffde15d8UL, 0xffddbdbeUL, 0xffdd65a4UL, 0xffdd0d8aUL, 0xffdcb570UL, 0xffdc5d56UL,
  0xffdc053cUL, 0xffdbad22UL, 0xffdb5508UL, 0xffdafceeUL, 0xffdaa4d4UL, 0xffda4cbaUL,
  0xffd9f4a0UL, 0xffd99c86UL, 0xffd9446cUL, 0xffd8ec52UL, 0xffd89438UL, 0xffd83c1eUL,
  0xffd7e404UL, 0xffd78beaUL, 0xffd733d0UL, 0xffd6dbb6UL, 0xffd6839cUL, 0xffd62b82UL,
  0xffd5d368UL, 0xffd57b4eUL, 0xffd52334UL, 0xffd4cb1aUL
 };












#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif


METHODDEF(void)
ycc_rgb_convert_argb (j_decompress_ptr cinfo,
                 JSAMPIMAGE input_buf, JDIMENSION input_row,
                 JSAMPARRAY output_buf, int num_rows)
{
  JDIMENSION num_cols = cinfo->output_width;
  JSAMPLE * range_limit = cinfo->sample_range_limit;

  SHIFT_TEMPS

  

  while (--num_rows >= 0) {
    JSAMPROW inptr0 = input_buf[0][input_row];
    JSAMPROW inptr1 = input_buf[1][input_row];
    JSAMPROW inptr2 = input_buf[2][input_row];
    input_row++;
    PRUint32 *outptr = (PRUint32 *) *output_buf++;
    for (JDIMENSION col = 0; col < num_cols; col++) {
      int y  = GETJSAMPLE(inptr0[col]);
      int cb = GETJSAMPLE(inptr1[col]);
      int cr = GETJSAMPLE(inptr2[col]);
      JSAMPLE * range_limit_y = range_limit + y;
      
      outptr[col] = 0xFF000000 |
                    ( range_limit_y[Cr_r_tab[cr]] << 16 ) |
                    ( range_limit_y[((int) RIGHT_SHIFT(Cb_g_tab[cb] + Cr_g_tab[cr], SCALEBITS))] << 8 ) |
                    ( range_limit_y[Cb_b_tab[cb]] );
    }
  }
}









static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width)
{
  
  JSAMPROW in = row + width*4;
  JSAMPROW out = in;

  for (PRUint32 i = width; i > 0; i--) {
    in -= 4;
    out -= 3;

    
    
    

    
    
    
    

    
    
    

    
    
    
    
  
    
    const PRUint32 iC = in[0];
    const PRUint32 iM = in[1];
    const PRUint32 iY = in[2];
    const PRUint32 iK = in[3];
    out[0] = iC*iK/255;   
    out[1] = iM*iK/255;   
    out[2] = iY*iK/255;   
  }
}
