








































#include "nsJPEGDecoder.h"
#include "ImageLogging.h"

#include "imgIContainerObserver.h"

#include "nsIInputStream.h"

#include "nspr.h"
#include "nsCRT.h"
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

static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width);

namespace mozilla {
namespace imagelib {

#if defined(PR_LOGGING)
PRLogModuleInfo *gJPEGlog = PR_NewLogModule("JPEGDecoder");
static PRLogModuleInfo *gJPEGDecoderAccountingLog = PR_NewLogModule("JPEGDecoderAccounting");
#else
#define gJPEGlog
#define gJPEGDecoderAccountingLog
#endif

static qcms_profile*
GetICCProfile(struct jpeg_decompress_struct &info)
{
  JOCTET* profilebuf;
  PRUint32 profileLength;
  qcms_profile* profile = nsnull;

  if (read_icc_profile(&info, &profilebuf, &profileLength)) {
    profile = qcms_profile_from_memory(profilebuf, profileLength);
    free(profilebuf);
  }

  return profile;
}

METHODDEF(void) init_source (j_decompress_ptr jd);
METHODDEF(boolean) fill_input_buffer (j_decompress_ptr jd);
METHODDEF(void) skip_input_data (j_decompress_ptr jd, long num_bytes);
METHODDEF(void) term_source (j_decompress_ptr jd);
METHODDEF(void) my_error_exit (j_common_ptr cinfo);


#define MAX_JPEG_MARKER_LENGTH  (((PRUint32)1 << 16) - 1)


nsJPEGDecoder::nsJPEGDecoder(RasterImage &aImage, imgIDecoderObserver* aObserver)
 : Decoder(aImage, aObserver)
{
  mState = JPEG_HEADER;
  mReading = true;
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

  mCMSMode = 0;

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("nsJPEGDecoder::nsJPEGDecoder: Creating JPEG decoder %p",
          this));
}

nsJPEGDecoder::~nsJPEGDecoder()
{
  
  mInfo.src = nsnull;
  jpeg_destroy_decompress(&mInfo);

  PR_FREEIF(mBackBuffer);
  if (mTransform)
    qcms_transform_release(mTransform);
  if (mInProfile)
    qcms_profile_release(mInProfile);

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("nsJPEGDecoder::~nsJPEGDecoder: Destroying JPEG decoder %p",
          this));
}

Telemetry::ID
nsJPEGDecoder::SpeedHistogram()
{
  return Telemetry::IMAGE_DECODE_SPEED_JPEG;
}

void
nsJPEGDecoder::InitInternal()
{
  mCMSMode = gfxPlatform::GetCMSMode();
  if ((mDecodeFlags & DECODER_NO_COLORSPACE_CONVERSION) != 0)
    mCMSMode = eCMSMode_Off;

  
  mInfo.err = jpeg_std_error(&mErr.pub);
  
  mErr.pub.error_exit = my_error_exit;
  
  if (setjmp(mErr.setjmp_buffer)) {
    


    PostDecoderError(NS_ERROR_FAILURE);
    return;
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
}

void
nsJPEGDecoder::FinishInternal()
{
  





  if ((mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER) &&
      (mState != JPEG_ERROR) &&
      !IsSizeDecode())
    this->Write(nsnull, 0);
}

void
nsJPEGDecoder::WriteInternal(const char *aBuffer, PRUint32 aCount)
{
  mSegment = (const JOCTET *)aBuffer;
  mSegmentLen = aCount;

  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  
  nsresult error_code;
  if ((error_code = setjmp(mErr.setjmp_buffer)) != 0) {
    if (error_code == NS_ERROR_FAILURE) {
      PostDataError();
      

      mState = JPEG_SINK_NON_JPEG_TRAILER;
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (setjmp returned NS_ERROR_FAILURE)"));
      return;
    } else {
      


      PostDecoderError(error_code);
      mState = JPEG_ERROR;
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (setjmp returned an error)"));
      return;
    }
  }

  PR_LOG(gJPEGlog, PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::Write -- processing JPEG data\n", this));

  switch (mState) {
  case JPEG_HEADER:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Write -- entering JPEG_HEADER case");

    
    if (jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED) {
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (JPEG_SUSPENDED)"));
      return; 
    }

    
    PostSize(mInfo.image_width, mInfo.image_height);
    if (HasError()) {
      
      
      mState = JPEG_ERROR;
      return;
    }

    
    if (IsSizeDecode())
      return;

    
    if (mCMSMode != eCMSMode_Off &&
        (mInProfile = GetICCProfile(mInfo)) != nsnull) {
      PRUint32 profileSpace = qcms_profile_get_color_space(mInProfile);
      bool mismatch = false;

#ifdef DEBUG_tor
      fprintf(stderr, "JPEG profileSpace: 0x%08X\n", profileSpace);
#endif
      switch (mInfo.jpeg_color_space) {
      case JCS_GRAYSCALE:
        if (profileSpace == icSigRgbData)
          mInfo.out_color_space = JCS_RGB;
        else if (profileSpace != icSigGrayData)
          mismatch = true;
        break;
      case JCS_RGB:
        if (profileSpace != icSigRgbData)
          mismatch =  true;
        break;
      case JCS_YCbCr:
        if (profileSpace == icSigRgbData)
          mInfo.out_color_space = JCS_RGB;
        else
	  
          mismatch = true;
        break;
      case JCS_CMYK:
      case JCS_YCCK:
	  
          mismatch = true;
        break;
      default:
        mState = JPEG_ERROR;
        PostDataError();
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (unknown colorpsace (1))"));
        return;
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
          PostDataError();
          PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                 ("} (unknown colorpsace (2))"));
          return;
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
        PostDataError();
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (unknown colorpsace (3))"));
        return;
        break;
      }
    }

    



    mInfo.buffered_image = jpeg_has_multiple_scans(&mInfo);

    
    jpeg_calc_output_dimensions(&mInfo);

    PRUint32 imagelength;
    if (NS_FAILED(mImage.EnsureFrame(0, 0, 0, mInfo.image_width, mInfo.image_height,
                                     gfxASurface::ImageFormatRGB24,
                                     &mImageData, &imagelength))) {
      mState = JPEG_ERROR;
      PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (could not initialize image frame)"));
      return;
    }

    PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
           ("        JPEGDecoderAccounting: nsJPEGDecoder::Write -- created image frame with %ux%u pixels",
            mInfo.image_width, mInfo.image_height));

    
    PostFrameStart();

    mState = JPEG_START_DECOMPRESS;
  }

  case JPEG_START_DECOMPRESS:
  {
    LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Write -- entering JPEG_START_DECOMPRESS case");
    

    


    mInfo.dct_method =  JDCT_ISLOW;
    mInfo.dither_mode = JDITHER_FS;
    mInfo.do_fancy_upsampling = TRUE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    
    if (jpeg_start_decompress(&mInfo) == FALSE) {
      PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
             ("} (I/O suspension after jpeg_start_decompress())"));
      return; 
    }

    
    if (!mTransform && (mCMSMode != eCMSMode_All) &&
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
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Write -- JPEG_DECOMPRESS_SEQUENTIAL case");
      
      bool suspend;
      OutputScanlines(&suspend);
      
      if (suspend) {
        PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
               ("} (I/O suspension after OutputScanlines() - SEQUENTIAL)"));
        return; 
      }
      
      
      NS_ASSERTION(mInfo.output_scanline == mInfo.output_height, "We didn't process all of the data!");
      mState = JPEG_DONE;
    }
  }

  case JPEG_DECOMPRESS_PROGRESSIVE:
  {
    if (mState == JPEG_DECOMPRESS_PROGRESSIVE)
    {
      LOG_SCOPE(gJPEGlog, "nsJPEGDecoder::Write -- JPEG_DECOMPRESS_PROGRESSIVE case");

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
            return; 
          }
        }

        if (mInfo.output_scanline == 0xffffff)
          mInfo.output_scanline = 0;

        bool suspend;
        OutputScanlines(&suspend);

        if (suspend) {
          if (mInfo.output_scanline == 0) {
            

            mInfo.output_scanline = 0xffffff;
          }
          PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                 ("} (I/O suspension after OutputScanlines() - PROGRESSIVE)"));
          return; 
        }

        if (mInfo.output_scanline == mInfo.output_height)
        {
          if (!jpeg_finish_output(&mInfo)) {
            PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
                   ("} (I/O suspension after jpeg_finish_output() - PROGRESSIVE)"));
            return; 
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
      return; 
    }

    mState = JPEG_SINK_NON_JPEG_TRAILER;

    
    break;
  }
  case JPEG_SINK_NON_JPEG_TRAILER:
    PR_LOG(gJPEGlog, PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::ProcessData -- entering JPEG_SINK_NON_JPEG_TRAILER case\n", this));

    break;

  case JPEG_ERROR:
    NS_ABORT_IF_FALSE(0, "Should always return immediately after error and not re-enter decoder");
  }

  PR_LOG(gJPEGDecoderAccountingLog, PR_LOG_DEBUG,
         ("} (end of function)"));
  return;
}

void
nsJPEGDecoder::NotifyDone()
{
  PostFrameStop();
  PostDecodeDone();
}

void
nsJPEGDecoder::OutputScanlines(bool* suspend)
{
  *suspend = false;

  const PRUint32 top = mInfo.output_scanline;

  while ((mInfo.output_scanline < mInfo.output_height)) {
      
      PRUint32 *imageRow = ((PRUint32*)mImageData) +
                           (mInfo.output_scanline * mInfo.output_width);

      if (mInfo.cconvert->color_convert == ycc_rgb_convert_argb) {
        
        if (jpeg_read_scanlines(&mInfo, (JSAMPARRAY)&imageRow, 1) != 1) {
          *suspend = true; 
          break;
        }
        continue; 
      }

      JSAMPROW sampleRow = (JSAMPROW)imageRow;
      if (mInfo.output_components == 3) {
        
        sampleRow += mInfo.output_width;
      }

          
      if (jpeg_read_scanlines(&mInfo, &sampleRow, 1) != 1) {
        *suspend = true; 
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
        if (mCMSMode == eCMSMode_All) {
          
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
      PostInvalidation(r);
  }

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
      return false; 

    decoder->mSegmentLen = 0;

    if (decoder->mBytesToSkip) {
      if (decoder->mBytesToSkip < new_buflen) {
        
        new_buffer += decoder->mBytesToSkip;
        new_buflen -= decoder->mBytesToSkip;
        decoder->mBytesToSkip = 0;
      } else {
        
        decoder->mBytesToSkip -= (size_t)new_buflen;
        return false; 
      }
    }

      decoder->mBackBufferUnreadLen = src->bytes_in_buffer;

    src->next_input_byte = new_buffer;
    src->bytes_in_buffer = (size_t)new_buflen;
    decoder->mReading = false;

    return true;
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
  decoder->mReading = true;

  return false;
}








METHODDEF(void)
term_source (j_decompress_ptr jd)
{
  nsJPEGDecoder *decoder = (nsJPEGDecoder *)(jd->client_data);

  
  
  NS_ABORT_IF_FALSE(decoder->mState != JPEG_ERROR,
                    "Calling term_source on a JPEG with mState == JPEG_ERROR!");

  
  decoder->NotifyDone();
}

} 
} 































#define SCALEBITS       16      /* speediest right-shift on some machines */




const int Cr_r_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
       -0xb3,       -0xb2,       -0xb1,       -0xaf,       -0xae,       -0xac,
       -0xab,       -0xaa,       -0xa8,       -0xa7,       -0xa5,       -0xa4,
       -0xa3,       -0xa1,       -0xa0,       -0x9e,       -0x9d,       -0x9c,
       -0x9a,       -0x99,       -0x97,       -0x96,       -0x95,       -0x93,
       -0x92,       -0x90,       -0x8f,       -0x8e,       -0x8c,       -0x8b,
       -0x89,       -0x88,       -0x87,       -0x85,       -0x84,       -0x82,
       -0x81,       -0x80,       -0x7e,       -0x7d,       -0x7b,       -0x7a,
       -0x79,       -0x77,       -0x76,       -0x74,       -0x73,       -0x72,
       -0x70,       -0x6f,       -0x6d,       -0x6c,       -0x6b,       -0x69,
       -0x68,       -0x66,       -0x65,       -0x64,       -0x62,       -0x61,
       -0x5f,       -0x5e,       -0x5d,       -0x5b,       -0x5a,       -0x58,
       -0x57,       -0x56,       -0x54,       -0x53,       -0x51,       -0x50,
       -0x4f,       -0x4d,       -0x4c,       -0x4a,       -0x49,       -0x48,
       -0x46,       -0x45,       -0x43,       -0x42,       -0x40,       -0x3f,
       -0x3e,       -0x3c,       -0x3b,       -0x39,       -0x38,       -0x37,
       -0x35,       -0x34,       -0x32,       -0x31,       -0x30,       -0x2e,
       -0x2d,       -0x2b,       -0x2a,       -0x29,       -0x27,       -0x26,
       -0x24,       -0x23,       -0x22,       -0x20,       -0x1f,       -0x1d,
       -0x1c,       -0x1b,       -0x19,       -0x18,       -0x16,       -0x15,
       -0x14,       -0x12,       -0x11,       -0x0f,       -0x0e,       -0x0d,
       -0x0b,       -0x0a,       -0x08,       -0x07,       -0x06,       -0x04,
       -0x03,       -0x01,        0x00,        0x01,        0x03,        0x04,
        0x06,        0x07,        0x08,        0x0a,        0x0b,        0x0d,
        0x0e,        0x0f,        0x11,        0x12,        0x14,        0x15,
        0x16,        0x18,        0x19,        0x1b,        0x1c,        0x1d,
        0x1f,        0x20,        0x22,        0x23,        0x24,        0x26,
        0x27,        0x29,        0x2a,        0x2b,        0x2d,        0x2e,
        0x30,        0x31,        0x32,        0x34,        0x35,        0x37,
        0x38,        0x39,        0x3b,        0x3c,        0x3e,        0x3f,
        0x40,        0x42,        0x43,        0x45,        0x46,        0x48,
        0x49,        0x4a,        0x4c,        0x4d,        0x4f,        0x50,
        0x51,        0x53,        0x54,        0x56,        0x57,        0x58,
        0x5a,        0x5b,        0x5d,        0x5e,        0x5f,        0x61,
        0x62,        0x64,        0x65,        0x66,        0x68,        0x69,
        0x6b,        0x6c,        0x6d,        0x6f,        0x70,        0x72,
        0x73,        0x74,        0x76,        0x77,        0x79,        0x7a,
        0x7b,        0x7d,        0x7e,        0x80,        0x81,        0x82,
        0x84,        0x85,        0x87,        0x88,        0x89,        0x8b,
        0x8c,        0x8e,        0x8f,        0x90,        0x92,        0x93,
        0x95,        0x96,        0x97,        0x99,        0x9a,        0x9c,
        0x9d,        0x9e,        0xa0,        0xa1,        0xa3,        0xa4,
        0xa5,        0xa7,        0xa8,        0xaa,        0xab,        0xac,
        0xae,        0xaf,        0xb1,        0xb2,
  };

const int Cb_b_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
       -0xe3,       -0xe1,       -0xdf,       -0xde,       -0xdc,       -0xda,
       -0xd8,       -0xd6,       -0xd5,       -0xd3,       -0xd1,       -0xcf,
       -0xce,       -0xcc,       -0xca,       -0xc8,       -0xc6,       -0xc5,
       -0xc3,       -0xc1,       -0xbf,       -0xbe,       -0xbc,       -0xba,
       -0xb8,       -0xb7,       -0xb5,       -0xb3,       -0xb1,       -0xaf,
       -0xae,       -0xac,       -0xaa,       -0xa8,       -0xa7,       -0xa5,
       -0xa3,       -0xa1,       -0x9f,       -0x9e,       -0x9c,       -0x9a,
       -0x98,       -0x97,       -0x95,       -0x93,       -0x91,       -0x90,
       -0x8e,       -0x8c,       -0x8a,       -0x88,       -0x87,       -0x85,
       -0x83,       -0x81,       -0x80,       -0x7e,       -0x7c,       -0x7a,
       -0x78,       -0x77,       -0x75,       -0x73,       -0x71,       -0x70,
       -0x6e,       -0x6c,       -0x6a,       -0x69,       -0x67,       -0x65,
       -0x63,       -0x61,       -0x60,       -0x5e,       -0x5c,       -0x5a,
       -0x59,       -0x57,       -0x55,       -0x53,       -0x52,       -0x50,
       -0x4e,       -0x4c,       -0x4a,       -0x49,       -0x47,       -0x45,
       -0x43,       -0x42,       -0x40,       -0x3e,       -0x3c,       -0x3a,
       -0x39,       -0x37,       -0x35,       -0x33,       -0x32,       -0x30,
       -0x2e,       -0x2c,       -0x2b,       -0x29,       -0x27,       -0x25,
       -0x23,       -0x22,       -0x20,       -0x1e,       -0x1c,       -0x1b,
       -0x19,       -0x17,       -0x15,       -0x13,       -0x12,       -0x10,
       -0x0e,       -0x0c,       -0x0b,       -0x09,       -0x07,       -0x05,
       -0x04,       -0x02,        0x00,        0x02,        0x04,        0x05,
        0x07,        0x09,        0x0b,        0x0c,        0x0e,        0x10,
        0x12,        0x13,        0x15,        0x17,        0x19,        0x1b,
        0x1c,        0x1e,        0x20,        0x22,        0x23,        0x25,
        0x27,        0x29,        0x2b,        0x2c,        0x2e,        0x30,
        0x32,        0x33,        0x35,        0x37,        0x39,        0x3a,
        0x3c,        0x3e,        0x40,        0x42,        0x43,        0x45,
        0x47,        0x49,        0x4a,        0x4c,        0x4e,        0x50,
        0x52,        0x53,        0x55,        0x57,        0x59,        0x5a,
        0x5c,        0x5e,        0x60,        0x61,        0x63,        0x65,
        0x67,        0x69,        0x6a,        0x6c,        0x6e,        0x70,
        0x71,        0x73,        0x75,        0x77,        0x78,        0x7a,
        0x7c,        0x7e,        0x80,        0x81,        0x83,        0x85,
        0x87,        0x88,        0x8a,        0x8c,        0x8e,        0x90,
        0x91,        0x93,        0x95,        0x97,        0x98,        0x9a,
        0x9c,        0x9e,        0x9f,        0xa1,        0xa3,        0xa5,
        0xa7,        0xa8,        0xaa,        0xac,        0xae,        0xaf,
        0xb1,        0xb3,        0xb5,        0xb7,        0xb8,        0xba,
        0xbc,        0xbe,        0xbf,        0xc1,        0xc3,        0xc5,
        0xc6,        0xc8,        0xca,        0xcc,        0xce,        0xcf,
        0xd1,        0xd3,        0xd5,        0xd6,        0xd8,        0xda,
        0xdc,        0xde,        0xdf,        0xe1,
  };

const int Cr_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x5b6900,    0x5ab22e,    0x59fb5c,    0x59448a,    0x588db8,    0x57d6e6,
    0x572014,    0x566942,    0x55b270,    0x54fb9e,    0x5444cc,    0x538dfa,
    0x52d728,    0x522056,    0x516984,    0x50b2b2,    0x4ffbe0,    0x4f450e,
    0x4e8e3c,    0x4dd76a,    0x4d2098,    0x4c69c6,    0x4bb2f4,    0x4afc22,
    0x4a4550,    0x498e7e,    0x48d7ac,    0x4820da,    0x476a08,    0x46b336,
    0x45fc64,    0x454592,    0x448ec0,    0x43d7ee,    0x43211c,    0x426a4a,
    0x41b378,    0x40fca6,    0x4045d4,    0x3f8f02,    0x3ed830,    0x3e215e,
    0x3d6a8c,    0x3cb3ba,    0x3bfce8,    0x3b4616,    0x3a8f44,    0x39d872,
    0x3921a0,    0x386ace,    0x37b3fc,    0x36fd2a,    0x364658,    0x358f86,
    0x34d8b4,    0x3421e2,    0x336b10,    0x32b43e,    0x31fd6c,    0x31469a,
    0x308fc8,    0x2fd8f6,    0x2f2224,    0x2e6b52,    0x2db480,    0x2cfdae,
    0x2c46dc,    0x2b900a,    0x2ad938,    0x2a2266,    0x296b94,    0x28b4c2,
    0x27fdf0,    0x27471e,    0x26904c,    0x25d97a,    0x2522a8,    0x246bd6,
    0x23b504,    0x22fe32,    0x224760,    0x21908e,    0x20d9bc,    0x2022ea,
    0x1f6c18,    0x1eb546,    0x1dfe74,    0x1d47a2,    0x1c90d0,    0x1bd9fe,
    0x1b232c,    0x1a6c5a,    0x19b588,    0x18feb6,    0x1847e4,    0x179112,
    0x16da40,    0x16236e,    0x156c9c,    0x14b5ca,    0x13fef8,    0x134826,
    0x129154,    0x11da82,    0x1123b0,    0x106cde,    0x0fb60c,    0x0eff3a,
    0x0e4868,    0x0d9196,    0x0cdac4,    0x0c23f2,    0x0b6d20,    0x0ab64e,
    0x09ff7c,    0x0948aa,    0x0891d8,    0x07db06,    0x072434,    0x066d62,
    0x05b690,    0x04ffbe,    0x0448ec,    0x03921a,    0x02db48,    0x022476,
    0x016da4,    0x00b6d2,    0x000000,   -0x00b6d2,   -0x016da4,   -0x022476,
   -0x02db48,   -0x03921a,   -0x0448ec,   -0x04ffbe,   -0x05b690,   -0x066d62,
   -0x072434,   -0x07db06,   -0x0891d8,   -0x0948aa,   -0x09ff7c,   -0x0ab64e,
   -0x0b6d20,   -0x0c23f2,   -0x0cdac4,   -0x0d9196,   -0x0e4868,   -0x0eff3a,
   -0x0fb60c,   -0x106cde,   -0x1123b0,   -0x11da82,   -0x129154,   -0x134826,
   -0x13fef8,   -0x14b5ca,   -0x156c9c,   -0x16236e,   -0x16da40,   -0x179112,
   -0x1847e4,   -0x18feb6,   -0x19b588,   -0x1a6c5a,   -0x1b232c,   -0x1bd9fe,
   -0x1c90d0,   -0x1d47a2,   -0x1dfe74,   -0x1eb546,   -0x1f6c18,   -0x2022ea,
   -0x20d9bc,   -0x21908e,   -0x224760,   -0x22fe32,   -0x23b504,   -0x246bd6,
   -0x2522a8,   -0x25d97a,   -0x26904c,   -0x27471e,   -0x27fdf0,   -0x28b4c2,
   -0x296b94,   -0x2a2266,   -0x2ad938,   -0x2b900a,   -0x2c46dc,   -0x2cfdae,
   -0x2db480,   -0x2e6b52,   -0x2f2224,   -0x2fd8f6,   -0x308fc8,   -0x31469a,
   -0x31fd6c,   -0x32b43e,   -0x336b10,   -0x3421e2,   -0x34d8b4,   -0x358f86,
   -0x364658,   -0x36fd2a,   -0x37b3fc,   -0x386ace,   -0x3921a0,   -0x39d872,
   -0x3a8f44,   -0x3b4616,   -0x3bfce8,   -0x3cb3ba,   -0x3d6a8c,   -0x3e215e,
   -0x3ed830,   -0x3f8f02,   -0x4045d4,   -0x40fca6,   -0x41b378,   -0x426a4a,
   -0x43211c,   -0x43d7ee,   -0x448ec0,   -0x454592,   -0x45fc64,   -0x46b336,
   -0x476a08,   -0x4820da,   -0x48d7ac,   -0x498e7e,   -0x4a4550,   -0x4afc22,
   -0x4bb2f4,   -0x4c69c6,   -0x4d2098,   -0x4dd76a,   -0x4e8e3c,   -0x4f450e,
   -0x4ffbe0,   -0x50b2b2,   -0x516984,   -0x522056,   -0x52d728,   -0x538dfa,
   -0x5444cc,   -0x54fb9e,   -0x55b270,   -0x566942,   -0x572014,   -0x57d6e6,
   -0x588db8,   -0x59448a,   -0x59fb5c,   -0x5ab22e,
 };

const int Cb_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x2c8d00,    0x2c34e6,    0x2bdccc,    0x2b84b2,    0x2b2c98,    0x2ad47e,
    0x2a7c64,    0x2a244a,    0x29cc30,    0x297416,    0x291bfc,    0x28c3e2,
    0x286bc8,    0x2813ae,    0x27bb94,    0x27637a,    0x270b60,    0x26b346,
    0x265b2c,    0x260312,    0x25aaf8,    0x2552de,    0x24fac4,    0x24a2aa,
    0x244a90,    0x23f276,    0x239a5c,    0x234242,    0x22ea28,    0x22920e,
    0x2239f4,    0x21e1da,    0x2189c0,    0x2131a6,    0x20d98c,    0x208172,
    0x202958,    0x1fd13e,    0x1f7924,    0x1f210a,    0x1ec8f0,    0x1e70d6,
    0x1e18bc,    0x1dc0a2,    0x1d6888,    0x1d106e,    0x1cb854,    0x1c603a,
    0x1c0820,    0x1bb006,    0x1b57ec,    0x1affd2,    0x1aa7b8,    0x1a4f9e,
    0x19f784,    0x199f6a,    0x194750,    0x18ef36,    0x18971c,    0x183f02,
    0x17e6e8,    0x178ece,    0x1736b4,    0x16de9a,    0x168680,    0x162e66,
    0x15d64c,    0x157e32,    0x152618,    0x14cdfe,    0x1475e4,    0x141dca,
    0x13c5b0,    0x136d96,    0x13157c,    0x12bd62,    0x126548,    0x120d2e,
    0x11b514,    0x115cfa,    0x1104e0,    0x10acc6,    0x1054ac,    0x0ffc92,
    0x0fa478,    0x0f4c5e,    0x0ef444,    0x0e9c2a,    0x0e4410,    0x0debf6,
    0x0d93dc,    0x0d3bc2,    0x0ce3a8,    0x0c8b8e,    0x0c3374,    0x0bdb5a,
    0x0b8340,    0x0b2b26,    0x0ad30c,    0x0a7af2,    0x0a22d8,    0x09cabe,
    0x0972a4,    0x091a8a,    0x08c270,    0x086a56,    0x08123c,    0x07ba22,
    0x076208,    0x0709ee,    0x06b1d4,    0x0659ba,    0x0601a0,    0x05a986,
    0x05516c,    0x04f952,    0x04a138,    0x04491e,    0x03f104,    0x0398ea,
    0x0340d0,    0x02e8b6,    0x02909c,    0x023882,    0x01e068,    0x01884e,
    0x013034,    0x00d81a,    0x008000,    0x0027e6,   -0x003034,   -0x00884e,
   -0x00e068,   -0x013882,   -0x01909c,   -0x01e8b6,   -0x0240d0,   -0x0298ea,
   -0x02f104,   -0x03491e,   -0x03a138,   -0x03f952,   -0x04516c,   -0x04a986,
   -0x0501a0,   -0x0559ba,   -0x05b1d4,   -0x0609ee,   -0x066208,   -0x06ba22,
   -0x07123c,   -0x076a56,   -0x07c270,   -0x081a8a,   -0x0872a4,   -0x08cabe,
   -0x0922d8,   -0x097af2,   -0x09d30c,   -0x0a2b26,   -0x0a8340,   -0x0adb5a,
   -0x0b3374,   -0x0b8b8e,   -0x0be3a8,   -0x0c3bc2,   -0x0c93dc,   -0x0cebf6,
   -0x0d4410,   -0x0d9c2a,   -0x0df444,   -0x0e4c5e,   -0x0ea478,   -0x0efc92,
   -0x0f54ac,   -0x0facc6,   -0x1004e0,   -0x105cfa,   -0x10b514,   -0x110d2e,
   -0x116548,   -0x11bd62,   -0x12157c,   -0x126d96,   -0x12c5b0,   -0x131dca,
   -0x1375e4,   -0x13cdfe,   -0x142618,   -0x147e32,   -0x14d64c,   -0x152e66,
   -0x158680,   -0x15de9a,   -0x1636b4,   -0x168ece,   -0x16e6e8,   -0x173f02,
   -0x17971c,   -0x17ef36,   -0x184750,   -0x189f6a,   -0x18f784,   -0x194f9e,
   -0x19a7b8,   -0x19ffd2,   -0x1a57ec,   -0x1ab006,   -0x1b0820,   -0x1b603a,
   -0x1bb854,   -0x1c106e,   -0x1c6888,   -0x1cc0a2,   -0x1d18bc,   -0x1d70d6,
   -0x1dc8f0,   -0x1e210a,   -0x1e7924,   -0x1ed13e,   -0x1f2958,   -0x1f8172,
   -0x1fd98c,   -0x2031a6,   -0x2089c0,   -0x20e1da,   -0x2139f4,   -0x21920e,
   -0x21ea28,   -0x224242,   -0x229a5c,   -0x22f276,   -0x234a90,   -0x23a2aa,
   -0x23fac4,   -0x2452de,   -0x24aaf8,   -0x250312,   -0x255b2c,   -0x25b346,
   -0x260b60,   -0x26637a,   -0x26bb94,   -0x2713ae,   -0x276bc8,   -0x27c3e2,
   -0x281bfc,   -0x287416,   -0x28cc30,   -0x29244a,   -0x297c64,   -0x29d47e,
   -0x2a2c98,   -0x2a84b2,   -0x2adccc,   -0x2b34e6,
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
