








































#include "nsJPEGDecoder.h"

#include "imgIContainerObserver.h"

#include "nsIInputStream.h"

#include "nspr.h"
#include "nsCRT.h"
#include "ImageLogging.h"
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


nsJPEGDecoder::nsJPEGDecoder()
{
  mState = JPEG_HEADER;
  mReading = PR_TRUE;
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
    if (NS_FAILED(mImage->EnsureCleanFrame(0, 0, 0, mInfo.image_width, mInfo.image_height,
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
      
      PRBool suspend;
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

        PRBool suspend;
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
nsJPEGDecoder::OutputScanlines(PRBool* suspend)
{
  *suspend = PR_FALSE;

  const PRUint32 top = mInfo.output_scanline;

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

  
  
  NS_ABORT_IF_FALSE(decoder->mState != JPEG_ERROR,
                    "Calling term_source on a JPEG with mState == JPEG_ERROR!");

  
  decoder->NotifyDone();
}

} 
} 































#define SCALEBITS       16      /* speediest right-shift on some machines */




const int Cr_r_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
  0xffffff4dU, 0xffffff4eU, 0xffffff4fU, 0xffffff51U, 0xffffff52U, 0xffffff54U, 
  0xffffff55U, 0xffffff56U, 0xffffff58U, 0xffffff59U, 0xffffff5bU, 0xffffff5cU, 
  0xffffff5dU, 0xffffff5fU, 0xffffff60U, 0xffffff62U, 0xffffff63U, 0xffffff64U, 
  0xffffff66U, 0xffffff67U, 0xffffff69U, 0xffffff6aU, 0xffffff6bU, 0xffffff6dU, 
  0xffffff6eU, 0xffffff70U, 0xffffff71U, 0xffffff72U, 0xffffff74U, 0xffffff75U, 
  0xffffff77U, 0xffffff78U, 0xffffff79U, 0xffffff7bU, 0xffffff7cU, 0xffffff7eU, 
  0xffffff7fU, 0xffffff80U, 0xffffff82U, 0xffffff83U, 0xffffff85U, 0xffffff86U, 
  0xffffff87U, 0xffffff89U, 0xffffff8aU, 0xffffff8cU, 0xffffff8dU, 0xffffff8eU, 
  0xffffff90U, 0xffffff91U, 0xffffff93U, 0xffffff94U, 0xffffff95U, 0xffffff97U, 
  0xffffff98U, 0xffffff9aU, 0xffffff9bU, 0xffffff9cU, 0xffffff9eU, 0xffffff9fU, 
  0xffffffa1U, 0xffffffa2U, 0xffffffa3U, 0xffffffa5U, 0xffffffa6U, 0xffffffa8U, 
  0xffffffa9U, 0xffffffaaU, 0xffffffacU, 0xffffffadU, 0xffffffafU, 0xffffffb0U, 
  0xffffffb1U, 0xffffffb3U, 0xffffffb4U, 0xffffffb6U, 0xffffffb7U, 0xffffffb8U, 
  0xffffffbaU, 0xffffffbbU, 0xffffffbdU, 0xffffffbeU, 0xffffffc0U, 0xffffffc1U, 
  0xffffffc2U, 0xffffffc4U, 0xffffffc5U, 0xffffffc7U, 0xffffffc8U, 0xffffffc9U, 
  0xffffffcbU, 0xffffffccU, 0xffffffceU, 0xffffffcfU, 0xffffffd0U, 0xffffffd2U, 
  0xffffffd3U, 0xffffffd5U, 0xffffffd6U, 0xffffffd7U, 0xffffffd9U, 0xffffffdaU, 
  0xffffffdcU, 0xffffffddU, 0xffffffdeU, 0xffffffe0U, 0xffffffe1U, 0xffffffe3U, 
  0xffffffe4U, 0xffffffe5U, 0xffffffe7U, 0xffffffe8U, 0xffffffeaU, 0xffffffebU, 
  0xffffffecU, 0xffffffeeU, 0xffffffefU, 0xfffffff1U, 0xfffffff2U, 0xfffffff3U, 
  0xfffffff5U, 0xfffffff6U, 0xfffffff8U, 0xfffffff9U, 0xfffffffaU, 0xfffffffcU, 
  0xfffffffdU, 0xffffffffU,       0x00U,       0x01U,       0x03U,       0x04U, 
        0x06U,       0x07U,       0x08U,       0x0aU,       0x0bU,       0x0dU, 
        0x0eU,       0x0fU,       0x11U,       0x12U,       0x14U,       0x15U, 
        0x16U,       0x18U,       0x19U,       0x1bU,       0x1cU,       0x1dU, 
        0x1fU,       0x20U,       0x22U,       0x23U,       0x24U,       0x26U, 
        0x27U,       0x29U,       0x2aU,       0x2bU,       0x2dU,       0x2eU, 
        0x30U,       0x31U,       0x32U,       0x34U,       0x35U,       0x37U, 
        0x38U,       0x39U,       0x3bU,       0x3cU,       0x3eU,       0x3fU, 
        0x40U,       0x42U,       0x43U,       0x45U,       0x46U,       0x48U, 
        0x49U,       0x4aU,       0x4cU,       0x4dU,       0x4fU,       0x50U, 
        0x51U,       0x53U,       0x54U,       0x56U,       0x57U,       0x58U, 
        0x5aU,       0x5bU,       0x5dU,       0x5eU,       0x5fU,       0x61U, 
        0x62U,       0x64U,       0x65U,       0x66U,       0x68U,       0x69U, 
        0x6bU,       0x6cU,       0x6dU,       0x6fU,       0x70U,       0x72U, 
        0x73U,       0x74U,       0x76U,       0x77U,       0x79U,       0x7aU, 
        0x7bU,       0x7dU,       0x7eU,       0x80U,       0x81U,       0x82U, 
        0x84U,       0x85U,       0x87U,       0x88U,       0x89U,       0x8bU, 
        0x8cU,       0x8eU,       0x8fU,       0x90U,       0x92U,       0x93U, 
        0x95U,       0x96U,       0x97U,       0x99U,       0x9aU,       0x9cU, 
        0x9dU,       0x9eU,       0xa0U,       0xa1U,       0xa3U,       0xa4U, 
        0xa5U,       0xa7U,       0xa8U,       0xaaU,       0xabU,       0xacU, 
        0xaeU,       0xafU,       0xb1U,       0xb2U
  };

const int Cb_b_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
  0xffffff1dU, 0xffffff1fU, 0xffffff21U, 0xffffff22U, 0xffffff24U, 0xffffff26U, 
  0xffffff28U, 0xffffff2aU, 0xffffff2bU, 0xffffff2dU, 0xffffff2fU, 0xffffff31U, 
  0xffffff32U, 0xffffff34U, 0xffffff36U, 0xffffff38U, 0xffffff3aU, 0xffffff3bU, 
  0xffffff3dU, 0xffffff3fU, 0xffffff41U, 0xffffff42U, 0xffffff44U, 0xffffff46U, 
  0xffffff48U, 0xffffff49U, 0xffffff4bU, 0xffffff4dU, 0xffffff4fU, 0xffffff51U, 
  0xffffff52U, 0xffffff54U, 0xffffff56U, 0xffffff58U, 0xffffff59U, 0xffffff5bU, 
  0xffffff5dU, 0xffffff5fU, 0xffffff61U, 0xffffff62U, 0xffffff64U, 0xffffff66U, 
  0xffffff68U, 0xffffff69U, 0xffffff6bU, 0xffffff6dU, 0xffffff6fU, 0xffffff70U, 
  0xffffff72U, 0xffffff74U, 0xffffff76U, 0xffffff78U, 0xffffff79U, 0xffffff7bU, 
  0xffffff7dU, 0xffffff7fU, 0xffffff80U, 0xffffff82U, 0xffffff84U, 0xffffff86U, 
  0xffffff88U, 0xffffff89U, 0xffffff8bU, 0xffffff8dU, 0xffffff8fU, 0xffffff90U, 
  0xffffff92U, 0xffffff94U, 0xffffff96U, 0xffffff97U, 0xffffff99U, 0xffffff9bU, 
  0xffffff9dU, 0xffffff9fU, 0xffffffa0U, 0xffffffa2U, 0xffffffa4U, 0xffffffa6U, 
  0xffffffa7U, 0xffffffa9U, 0xffffffabU, 0xffffffadU, 0xffffffaeU, 0xffffffb0U, 
  0xffffffb2U, 0xffffffb4U, 0xffffffb6U, 0xffffffb7U, 0xffffffb9U, 0xffffffbbU, 
  0xffffffbdU, 0xffffffbeU, 0xffffffc0U, 0xffffffc2U, 0xffffffc4U, 0xffffffc6U, 
  0xffffffc7U, 0xffffffc9U, 0xffffffcbU, 0xffffffcdU, 0xffffffceU, 0xffffffd0U, 
  0xffffffd2U, 0xffffffd4U, 0xffffffd5U, 0xffffffd7U, 0xffffffd9U, 0xffffffdbU, 
  0xffffffddU, 0xffffffdeU, 0xffffffe0U, 0xffffffe2U, 0xffffffe4U, 0xffffffe5U, 
  0xffffffe7U, 0xffffffe9U, 0xffffffebU, 0xffffffedU, 0xffffffeeU, 0xfffffff0U, 
  0xfffffff2U, 0xfffffff4U, 0xfffffff5U, 0xfffffff7U, 0xfffffff9U, 0xfffffffbU, 
  0xfffffffcU, 0xfffffffeU,       0x00U,       0x02U,       0x04U,       0x05U, 
        0x07U,       0x09U,       0x0bU,       0x0cU,       0x0eU,       0x10U, 
        0x12U,       0x13U,       0x15U,       0x17U,       0x19U,       0x1bU, 
        0x1cU,       0x1eU,       0x20U,       0x22U,       0x23U,       0x25U, 
        0x27U,       0x29U,       0x2bU,       0x2cU,       0x2eU,       0x30U, 
        0x32U,       0x33U,       0x35U,       0x37U,       0x39U,       0x3aU, 
        0x3cU,       0x3eU,       0x40U,       0x42U,       0x43U,       0x45U, 
        0x47U,       0x49U,       0x4aU,       0x4cU,       0x4eU,       0x50U, 
        0x52U,       0x53U,       0x55U,       0x57U,       0x59U,       0x5aU, 
        0x5cU,       0x5eU,       0x60U,       0x61U,       0x63U,       0x65U, 
        0x67U,       0x69U,       0x6aU,       0x6cU,       0x6eU,       0x70U, 
        0x71U,       0x73U,       0x75U,       0x77U,       0x78U,       0x7aU, 
        0x7cU,       0x7eU,       0x80U,       0x81U,       0x83U,       0x85U, 
        0x87U,       0x88U,       0x8aU,       0x8cU,       0x8eU,       0x90U, 
        0x91U,       0x93U,       0x95U,       0x97U,       0x98U,       0x9aU, 
        0x9cU,       0x9eU,       0x9fU,       0xa1U,       0xa3U,       0xa5U, 
        0xa7U,       0xa8U,       0xaaU,       0xacU,       0xaeU,       0xafU, 
        0xb1U,       0xb3U,       0xb5U,       0xb7U,       0xb8U,       0xbaU, 
        0xbcU,       0xbeU,       0xbfU,       0xc1U,       0xc3U,       0xc5U, 
        0xc6U,       0xc8U,       0xcaU,       0xccU,       0xceU,       0xcfU, 
        0xd1U,       0xd3U,       0xd5U,       0xd6U,       0xd8U,       0xdaU, 
        0xdcU,       0xdeU,       0xdfU,       0xe1U
  };

const int Cr_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x5b6900U,   0x5ab22eU,   0x59fb5cU,   0x59448aU,   0x588db8U,   0x57d6e6U, 
    0x572014U,   0x566942U,   0x55b270U,   0x54fb9eU,   0x5444ccU,   0x538dfaU, 
    0x52d728U,   0x522056U,   0x516984U,   0x50b2b2U,   0x4ffbe0U,   0x4f450eU, 
    0x4e8e3cU,   0x4dd76aU,   0x4d2098U,   0x4c69c6U,   0x4bb2f4U,   0x4afc22U, 
    0x4a4550U,   0x498e7eU,   0x48d7acU,   0x4820daU,   0x476a08U,   0x46b336U, 
    0x45fc64U,   0x454592U,   0x448ec0U,   0x43d7eeU,   0x43211cU,   0x426a4aU, 
    0x41b378U,   0x40fca6U,   0x4045d4U,   0x3f8f02U,   0x3ed830U,   0x3e215eU, 
    0x3d6a8cU,   0x3cb3baU,   0x3bfce8U,   0x3b4616U,   0x3a8f44U,   0x39d872U, 
    0x3921a0U,   0x386aceU,   0x37b3fcU,   0x36fd2aU,   0x364658U,   0x358f86U, 
    0x34d8b4U,   0x3421e2U,   0x336b10U,   0x32b43eU,   0x31fd6cU,   0x31469aU, 
    0x308fc8U,   0x2fd8f6U,   0x2f2224U,   0x2e6b52U,   0x2db480U,   0x2cfdaeU, 
    0x2c46dcU,   0x2b900aU,   0x2ad938U,   0x2a2266U,   0x296b94U,   0x28b4c2U, 
    0x27fdf0U,   0x27471eU,   0x26904cU,   0x25d97aU,   0x2522a8U,   0x246bd6U, 
    0x23b504U,   0x22fe32U,   0x224760U,   0x21908eU,   0x20d9bcU,   0x2022eaU, 
    0x1f6c18U,   0x1eb546U,   0x1dfe74U,   0x1d47a2U,   0x1c90d0U,   0x1bd9feU, 
    0x1b232cU,   0x1a6c5aU,   0x19b588U,   0x18feb6U,   0x1847e4U,   0x179112U, 
    0x16da40U,   0x16236eU,   0x156c9cU,   0x14b5caU,   0x13fef8U,   0x134826U, 
    0x129154U,   0x11da82U,   0x1123b0U,   0x106cdeU,    0xfb60cU,    0xeff3aU, 
     0xe4868U,    0xd9196U,    0xcdac4U,    0xc23f2U,    0xb6d20U,    0xab64eU, 
     0x9ff7cU,    0x948aaU,    0x891d8U,    0x7db06U,    0x72434U,    0x66d62U, 
     0x5b690U,    0x4ffbeU,    0x448ecU,    0x3921aU,    0x2db48U,    0x22476U, 
     0x16da4U,     0xb6d2U,        0x0U, 0xffff492eU, 0xfffe925cU, 0xfffddb8aU, 
  0xfffd24b8U, 0xfffc6de6U, 0xfffbb714U, 0xfffb0042U, 0xfffa4970U, 0xfff9929eU, 
  0xfff8dbccU, 0xfff824faU, 0xfff76e28U, 0xfff6b756U, 0xfff60084U, 0xfff549b2U, 
  0xfff492e0U, 0xfff3dc0eU, 0xfff3253cU, 0xfff26e6aU, 0xfff1b798U, 0xfff100c6U, 
  0xfff049f4U, 0xffef9322U, 0xffeedc50U, 0xffee257eU, 0xffed6eacU, 0xffecb7daU, 
  0xffec0108U, 0xffeb4a36U, 0xffea9364U, 0xffe9dc92U, 0xffe925c0U, 0xffe86eeeU, 
  0xffe7b81cU, 0xffe7014aU, 0xffe64a78U, 0xffe593a6U, 0xffe4dcd4U, 0xffe42602U, 
  0xffe36f30U, 0xffe2b85eU, 0xffe2018cU, 0xffe14abaU, 0xffe093e8U, 0xffdfdd16U, 
  0xffdf2644U, 0xffde6f72U, 0xffddb8a0U, 0xffdd01ceU, 0xffdc4afcU, 0xffdb942aU, 
  0xffdadd58U, 0xffda2686U, 0xffd96fb4U, 0xffd8b8e2U, 0xffd80210U, 0xffd74b3eU, 
  0xffd6946cU, 0xffd5dd9aU, 0xffd526c8U, 0xffd46ff6U, 0xffd3b924U, 0xffd30252U, 
  0xffd24b80U, 0xffd194aeU, 0xffd0dddcU, 0xffd0270aU, 0xffcf7038U, 0xffceb966U, 
  0xffce0294U, 0xffcd4bc2U, 0xffcc94f0U, 0xffcbde1eU, 0xffcb274cU, 0xffca707aU, 
  0xffc9b9a8U, 0xffc902d6U, 0xffc84c04U, 0xffc79532U, 0xffc6de60U, 0xffc6278eU, 
  0xffc570bcU, 0xffc4b9eaU, 0xffc40318U, 0xffc34c46U, 0xffc29574U, 0xffc1dea2U, 
  0xffc127d0U, 0xffc070feU, 0xffbfba2cU, 0xffbf035aU, 0xffbe4c88U, 0xffbd95b6U, 
  0xffbcdee4U, 0xffbc2812U, 0xffbb7140U, 0xffbaba6eU, 0xffba039cU, 0xffb94ccaU, 
  0xffb895f8U, 0xffb7df26U, 0xffb72854U, 0xffb67182U, 0xffb5bab0U, 0xffb503deU, 
  0xffb44d0cU, 0xffb3963aU, 0xffb2df68U, 0xffb22896U, 0xffb171c4U, 0xffb0baf2U, 
  0xffb00420U, 0xffaf4d4eU, 0xffae967cU, 0xffaddfaaU, 0xffad28d8U, 0xffac7206U, 
  0xffabbb34U, 0xffab0462U, 0xffaa4d90U, 0xffa996beU, 0xffa8dfecU, 0xffa8291aU, 
  0xffa77248U, 0xffa6bb76U, 0xffa604a4U, 0xffa54dd2U
 };

const int Cb_g_tab[(MAXJSAMPLE+1) * sizeof(int)] ={
    0x2c8d00U,   0x2c34e6U,   0x2bdcccU,   0x2b84b2U,   0x2b2c98U,   0x2ad47eU, 
    0x2a7c64U,   0x2a244aU,   0x29cc30U,   0x297416U,   0x291bfcU,   0x28c3e2U, 
    0x286bc8U,   0x2813aeU,   0x27bb94U,   0x27637aU,   0x270b60U,   0x26b346U, 
    0x265b2cU,   0x260312U,   0x25aaf8U,   0x2552deU,   0x24fac4U,   0x24a2aaU, 
    0x244a90U,   0x23f276U,   0x239a5cU,   0x234242U,   0x22ea28U,   0x22920eU, 
    0x2239f4U,   0x21e1daU,   0x2189c0U,   0x2131a6U,   0x20d98cU,   0x208172U, 
    0x202958U,   0x1fd13eU,   0x1f7924U,   0x1f210aU,   0x1ec8f0U,   0x1e70d6U, 
    0x1e18bcU,   0x1dc0a2U,   0x1d6888U,   0x1d106eU,   0x1cb854U,   0x1c603aU, 
    0x1c0820U,   0x1bb006U,   0x1b57ecU,   0x1affd2U,   0x1aa7b8U,   0x1a4f9eU, 
    0x19f784U,   0x199f6aU,   0x194750U,   0x18ef36U,   0x18971cU,   0x183f02U, 
    0x17e6e8U,   0x178eceU,   0x1736b4U,   0x16de9aU,   0x168680U,   0x162e66U, 
    0x15d64cU,   0x157e32U,   0x152618U,   0x14cdfeU,   0x1475e4U,   0x141dcaU, 
    0x13c5b0U,   0x136d96U,   0x13157cU,   0x12bd62U,   0x126548U,   0x120d2eU, 
    0x11b514U,   0x115cfaU,   0x1104e0U,   0x10acc6U,   0x1054acU,    0xffc92U, 
     0xfa478U,    0xf4c5eU,    0xef444U,    0xe9c2aU,    0xe4410U,    0xdebf6U, 
     0xd93dcU,    0xd3bc2U,    0xce3a8U,    0xc8b8eU,    0xc3374U,    0xbdb5aU, 
     0xb8340U,    0xb2b26U,    0xad30cU,    0xa7af2U,    0xa22d8U,    0x9cabeU, 
     0x972a4U,    0x91a8aU,    0x8c270U,    0x86a56U,    0x8123cU,    0x7ba22U, 
     0x76208U,    0x709eeU,    0x6b1d4U,    0x659baU,    0x601a0U,    0x5a986U, 
     0x5516cU,    0x4f952U,    0x4a138U,    0x4491eU,    0x3f104U,    0x398eaU, 
     0x340d0U,    0x2e8b6U,    0x2909cU,    0x23882U,    0x1e068U,    0x1884eU, 
     0x13034U,     0xd81aU,     0x8000U,     0x27e6U, 0xffffcfccU, 0xffff77b2U,
  0xffff1f98U, 0xfffec77eU, 0xfffe6f64U, 0xfffe174aU, 0xfffdbf30U, 0xfffd6716U,
  0xfffd0efcU, 0xfffcb6e2U, 0xfffc5ec8U, 0xfffc06aeU, 0xfffbae94U, 0xfffb567aU,
  0xfffafe60U, 0xfffaa646U, 0xfffa4e2cU, 0xfff9f612U, 0xfff99df8U, 0xfff945deU,
  0xfff8edc4U, 0xfff895aaU, 0xfff83d90U, 0xfff7e576U, 0xfff78d5cU, 0xfff73542U,
  0xfff6dd28U, 0xfff6850eU, 0xfff62cf4U, 0xfff5d4daU, 0xfff57cc0U, 0xfff524a6U,
  0xfff4cc8cU, 0xfff47472U, 0xfff41c58U, 0xfff3c43eU, 0xfff36c24U, 0xfff3140aU,
  0xfff2bbf0U, 0xfff263d6U, 0xfff20bbcU, 0xfff1b3a2U, 0xfff15b88U, 0xfff1036eU,
  0xfff0ab54U, 0xfff0533aU, 0xffeffb20U, 0xffefa306U, 0xffef4aecU, 0xffeef2d2U,
  0xffee9ab8U, 0xffee429eU, 0xffedea84U, 0xffed926aU, 0xffed3a50U, 0xffece236U,
  0xffec8a1cU, 0xffec3202U, 0xffebd9e8U, 0xffeb81ceU, 0xffeb29b4U, 0xffead19aU,
  0xffea7980U, 0xffea2166U, 0xffe9c94cU, 0xffe97132U, 0xffe91918U, 0xffe8c0feU,
  0xffe868e4U, 0xffe810caU, 0xffe7b8b0U, 0xffe76096U, 0xffe7087cU, 0xffe6b062U,
  0xffe65848U, 0xffe6002eU, 0xffe5a814U, 0xffe54ffaU, 0xffe4f7e0U, 0xffe49fc6U,
  0xffe447acU, 0xffe3ef92U, 0xffe39778U, 0xffe33f5eU, 0xffe2e744U, 0xffe28f2aU,
  0xffe23710U, 0xffe1def6U, 0xffe186dcU, 0xffe12ec2U, 0xffe0d6a8U, 0xffe07e8eU,
  0xffe02674U, 0xffdfce5aU, 0xffdf7640U, 0xffdf1e26U, 0xffdec60cU, 0xffde6df2U,
  0xffde15d8U, 0xffddbdbeU, 0xffdd65a4U, 0xffdd0d8aU, 0xffdcb570U, 0xffdc5d56U,
  0xffdc053cU, 0xffdbad22U, 0xffdb5508U, 0xffdafceeU, 0xffdaa4d4U, 0xffda4cbaU,
  0xffd9f4a0U, 0xffd99c86U, 0xffd9446cU, 0xffd8ec52U, 0xffd89438U, 0xffd83c1eU,
  0xffd7e404U, 0xffd78beaU, 0xffd733d0U, 0xffd6dbb6U, 0xffd6839cU, 0xffd62b82U,
  0xffd5d368U, 0xffd57b4eU, 0xffd52334U, 0xffd4cb1aU
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
