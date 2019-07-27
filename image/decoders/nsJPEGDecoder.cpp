





#include "ImageLogging.h"
#include "nsJPEGDecoder.h"
#include "Orientation.h"
#include "EXIF.h"

#include "nsIInputStream.h"

#include "nspr.h"
#include "nsCRT.h"
#include "gfxColor.h"

#include "jerror.h"

#include "gfxPlatform.h"
#include "mozilla/Endian.h"
#include "mozilla/Telemetry.h"

extern "C" {
#include "iccjpeg.h"
}

#if MOZ_BIG_ENDIAN
#define MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB JCS_EXT_XRGB
#else
#define MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB JCS_EXT_BGRX
#endif

static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width);

namespace mozilla {
namespace image {

#if defined(PR_LOGGING)
static PRLogModuleInfo*
GetJPEGLog()
{
  static PRLogModuleInfo* sJPEGLog;
  if (!sJPEGLog) {
    sJPEGLog = PR_NewLogModule("JPEGDecoder");
  }
  return sJPEGLog;
}

static PRLogModuleInfo*
GetJPEGDecoderAccountingLog()
{
  static PRLogModuleInfo* sJPEGDecoderAccountingLog;
  if (!sJPEGDecoderAccountingLog) {
    sJPEGDecoderAccountingLog = PR_NewLogModule("JPEGDecoderAccounting");
  }
  return sJPEGDecoderAccountingLog;
}
#else
#define GetJPEGLog()
#define GetJPEGDecoderAccountingLog()
#endif

static qcms_profile*
GetICCProfile(struct jpeg_decompress_struct& info)
{
  JOCTET* profilebuf;
  uint32_t profileLength;
  qcms_profile* profile = nullptr;

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


#define MAX_JPEG_MARKER_LENGTH  (((uint32_t)1 << 16) - 1)


nsJPEGDecoder::nsJPEGDecoder(RasterImage* aImage,
                             Decoder::DecodeStyle aDecodeStyle)
 : Decoder(aImage)
 , mDecodeStyle(aDecodeStyle)
{
  mState = JPEG_HEADER;
  mReading = true;
  mImageData = nullptr;

  mBytesToSkip = 0;
  memset(&mInfo, 0, sizeof(jpeg_decompress_struct));
  memset(&mSourceMgr, 0, sizeof(mSourceMgr));
  mInfo.client_data = (void*)this;

  mSegment = nullptr;
  mSegmentLen = 0;

  mBackBuffer = nullptr;
  mBackBufferLen = mBackBufferSize = mBackBufferUnreadLen = 0;

  mInProfile = nullptr;
  mTransform = nullptr;

  mCMSMode = 0;

  PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
         ("nsJPEGDecoder::nsJPEGDecoder: Creating JPEG decoder %p",
          this));
}

nsJPEGDecoder::~nsJPEGDecoder()
{
  
  mInfo.src = nullptr;
  jpeg_destroy_decompress(&mInfo);

  PR_FREEIF(mBackBuffer);
  if (mTransform) {
    qcms_transform_release(mTransform);
  }
  if (mInProfile) {
    qcms_profile_release(mInProfile);
  }

  PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
         ("nsJPEGDecoder::~nsJPEGDecoder: Destroying JPEG decoder %p",
          this));
}

Telemetry::ID
nsJPEGDecoder::SpeedHistogram()
{
  return Telemetry::IMAGE_DECODE_SPEED_JPEG;
}

nsresult
nsJPEGDecoder::SetTargetSize(const nsIntSize& aSize)
{
  
  if (MOZ_UNLIKELY(aSize.width <= 0 || aSize.height <= 0)) {
    return NS_ERROR_FAILURE;
  }

  
  mDownscaler.emplace(aSize);

  return NS_OK;
}

void
nsJPEGDecoder::InitInternal()
{
  mCMSMode = gfxPlatform::GetCMSMode();
  if (GetDecodeFlags() & imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION) {
    mCMSMode = eCMSMode_Off;
  }

  
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

  
  for (uint32_t m = 0; m < 16; m++) {
    jpeg_save_markers(&mInfo, JPEG_APP0 + m, 0xFFFF);
  }
}

void
nsJPEGDecoder::FinishInternal()
{
  
  if ((mState != JPEG_DONE && mState != JPEG_SINK_NON_JPEG_TRAILER) &&
      (mState != JPEG_ERROR) &&
      !IsSizeDecode()) {
    mState = JPEG_DONE;
  }
}

void
nsJPEGDecoder::WriteInternal(const char* aBuffer, uint32_t aCount)
{
  mSegment = (const JOCTET*)aBuffer;
  mSegmentLen = aCount;

  MOZ_ASSERT(!HasError(), "Shouldn't call WriteInternal after error!");

  
  nsresult error_code;
  
  
  if ((error_code = (nsresult)setjmp(mErr.setjmp_buffer)) != NS_OK) {
    if (error_code == NS_ERROR_FAILURE) {
      PostDataError();
      
      
      mState = JPEG_SINK_NON_JPEG_TRAILER;
      PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
             ("} (setjmp returned NS_ERROR_FAILURE)"));
      return;
    } else {
      
      
      
      PostDecoderError(error_code);
      mState = JPEG_ERROR;
      PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
             ("} (setjmp returned an error)"));
      return;
    }
  }

  PR_LOG(GetJPEGLog(), PR_LOG_DEBUG,
         ("[this=%p] nsJPEGDecoder::Write -- processing JPEG data\n", this));

  switch (mState) {
    case JPEG_HEADER: {
      LOG_SCOPE(GetJPEGLog(), "nsJPEGDecoder::Write -- entering JPEG_HEADER"
                " case");

      
      if (jpeg_read_header(&mInfo, TRUE) == JPEG_SUSPENDED) {
        PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
               ("} (JPEG_SUSPENDED)"));
        return; 
      }

      int sampleSize = mImage->GetRequestedSampleSize();
      if (sampleSize > 0) {
        mInfo.scale_num = 1;
        mInfo.scale_denom = sampleSize;
      }

      
      jpeg_calc_output_dimensions(&mInfo);

      
      PostSize(mInfo.output_width, mInfo.output_height,
               ReadOrientationFromEXIF());
      if (HasError()) {
        
        mState = JPEG_ERROR;
        return;
      }

      
      if (IsSizeDecode()) {
        return;
      }

      
      if (mCMSMode != eCMSMode_Off &&
          (mInProfile = GetICCProfile(mInfo)) != nullptr) {
        uint32_t profileSpace = qcms_profile_get_color_space(mInProfile);
        bool mismatch = false;

#ifdef DEBUG_tor
      fprintf(stderr, "JPEG profileSpace: 0x%08X\n", profileSpace);
#endif
      switch (mInfo.jpeg_color_space) {
        case JCS_GRAYSCALE:
          if (profileSpace == icSigRgbData) {
            mInfo.out_color_space = JCS_RGB;
          } else if (profileSpace != icSigGrayData) {
            mismatch = true;
          }
          break;
        case JCS_RGB:
          if (profileSpace != icSigRgbData) {
            mismatch =  true;
          }
          break;
        case JCS_YCbCr:
          if (profileSpace == icSigRgbData) {
            mInfo.out_color_space = JCS_RGB;
          } else {
            
            mismatch = true;
          }
          break;
        case JCS_CMYK:
        case JCS_YCCK:
            
            mismatch = true;
          break;
        default:
          mState = JPEG_ERROR;
          PostDataError();
          PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
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
            PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
                   ("} (unknown colorpsace (2))"));
            return;
        }
#if 0
        
        
        

        
        if (mInfo.out_color_space == JCS_CMYK) {
          type |= FLAVOR_SH(mInfo.saw_Adobe_marker ? 1 : 0);
        }
#endif

        if (gfxPlatform::GetCMSOutputProfile()) {

          
          int intent = gfxPlatform::GetRenderingIntent();
          if (intent == -1) {
            intent = qcms_profile_get_rendering_intent(mInProfile);
          }

          
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
          
          
          if (mCMSMode != eCMSMode_All) {
              mInfo.out_color_space = MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB;
              mInfo.out_color_components = 4;
          } else {
              mInfo.out_color_space = JCS_RGB;
          }
          break;
        case JCS_CMYK:
        case JCS_YCCK:
          
          mInfo.out_color_space = JCS_CMYK;
          break;
        default:
          mState = JPEG_ERROR;
          PostDataError();
          PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
                 ("} (unknown colorpsace (3))"));
          return;
          break;
      }
    }

    
    
    mInfo.buffered_image = mDecodeStyle == PROGRESSIVE &&
                           jpeg_has_multiple_scans(&mInfo);

    if (!mImageData) {
      mState = JPEG_ERROR;
      PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
      PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
             ("} (could not initialize image frame)"));
      return;
    }

    if (mDownscaler) {
      nsresult rv = mDownscaler->BeginFrame(GetSize(),
                                            mImageData,
                                             false);
      if (NS_FAILED(rv)) {
        mState = JPEG_ERROR;
        return;
      }
    }


    PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
           ("        JPEGDecoderAccounting: nsJPEGDecoder::"
            "Write -- created image frame with %ux%u pixels",
            mInfo.output_width, mInfo.output_height));

    mState = JPEG_START_DECOMPRESS;
  }

  case JPEG_START_DECOMPRESS: {
    LOG_SCOPE(GetJPEGLog(), "nsJPEGDecoder::Write -- entering"
                            " JPEG_START_DECOMPRESS case");
    

    
    

    mInfo.dct_method =  JDCT_ISLOW;
    mInfo.dither_mode = JDITHER_FS;
    mInfo.do_fancy_upsampling = TRUE;
    mInfo.enable_2pass_quant = FALSE;
    mInfo.do_block_smoothing = TRUE;

    
    if (jpeg_start_decompress(&mInfo) == FALSE) {
      PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
             ("} (I/O suspension after jpeg_start_decompress())"));
      return; 
    }


    
    mState = mInfo.buffered_image ?
             JPEG_DECOMPRESS_PROGRESSIVE : JPEG_DECOMPRESS_SEQUENTIAL;
  }

  case JPEG_DECOMPRESS_SEQUENTIAL: {
    if (mState == JPEG_DECOMPRESS_SEQUENTIAL) {
      LOG_SCOPE(GetJPEGLog(), "nsJPEGDecoder::Write -- "
                              "JPEG_DECOMPRESS_SEQUENTIAL case");

      bool suspend;
      OutputScanlines(&suspend);

      if (suspend) {
        PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
               ("} (I/O suspension after OutputScanlines() - SEQUENTIAL)"));
        return; 
      }

      
      NS_ASSERTION(mInfo.output_scanline == mInfo.output_height,
                   "We didn't process all of the data!");
      mState = JPEG_DONE;
    }
  }

  case JPEG_DECOMPRESS_PROGRESSIVE: {
    if (mState == JPEG_DECOMPRESS_PROGRESSIVE) {
      LOG_SCOPE(GetJPEGLog(),
                "nsJPEGDecoder::Write -- JPEG_DECOMPRESS_PROGRESSIVE case");

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
            PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
                   ("} (I/O suspension after jpeg_start_output() -"
                    " PROGRESSIVE)"));
            return; 
          }
        }

        if (mInfo.output_scanline == 0xffffff) {
          mInfo.output_scanline = 0;
        }

        bool suspend;
        OutputScanlines(&suspend);

        if (suspend) {
          if (mInfo.output_scanline == 0) {
            
            
            mInfo.output_scanline = 0xffffff;
          }
          PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
                 ("} (I/O suspension after OutputScanlines() - PROGRESSIVE)"));
          return; 
        }

        if (mInfo.output_scanline == mInfo.output_height) {
          if (!jpeg_finish_output(&mInfo)) {
            PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
                   ("} (I/O suspension after jpeg_finish_output() -"
                    " PROGRESSIVE)"));
            return; 
          }

          if (jpeg_input_complete(&mInfo) &&
              (mInfo.input_scan_number == mInfo.output_scan_number))
            break;

          mInfo.output_scanline = 0;
          if (mDownscaler) {
            mDownscaler->ResetForNextProgressivePass();
          }
        }
      }

      mState = JPEG_DONE;
    }
  }

  case JPEG_DONE: {
    LOG_SCOPE(GetJPEGLog(), "nsJPEGDecoder::ProcessData -- entering"
                            " JPEG_DONE case");

    

    if (jpeg_finish_decompress(&mInfo) == FALSE) {
      PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
             ("} (I/O suspension after jpeg_finish_decompress() - DONE)"));
      return; 
    }

    mState = JPEG_SINK_NON_JPEG_TRAILER;

    
    break;
  }
  case JPEG_SINK_NON_JPEG_TRAILER:
    PR_LOG(GetJPEGLog(), PR_LOG_DEBUG,
           ("[this=%p] nsJPEGDecoder::ProcessData -- entering"
            " JPEG_SINK_NON_JPEG_TRAILER case\n", this));

    break;

  case JPEG_ERROR:
    MOZ_ASSERT(false,
               "Should always return immediately after error and not re-enter "
               "decoder");
  }

  PR_LOG(GetJPEGDecoderAccountingLog(), PR_LOG_DEBUG,
         ("} (end of function)"));
  return;
}

Orientation
nsJPEGDecoder::ReadOrientationFromEXIF()
{
  jpeg_saved_marker_ptr marker;

  
  for (marker = mInfo.marker_list ; marker != nullptr ; marker = marker->next) {
    if (marker->marker == JPEG_APP0 + 1) {
      break;
    }
  }

  
  if (!marker) {
    return Orientation();
  }

  
  EXIFData exif = EXIFParser::Parse(marker->data,
                                    static_cast<uint32_t>(marker->data_length));
  return exif.orientation;
}

void
nsJPEGDecoder::NotifyDone()
{
  PostFrameStop(Opacity::OPAQUE);
  PostDecodeDone();
}

void
nsJPEGDecoder::OutputScanlines(bool* suspend)
{
  *suspend = false;

  const uint32_t top = mInfo.output_scanline;

  while ((mInfo.output_scanline < mInfo.output_height)) {
      uint32_t* imageRow = nullptr;
      if (mDownscaler) {
        imageRow = reinterpret_cast<uint32_t*>(mDownscaler->RowBuffer());
      } else {
        imageRow = reinterpret_cast<uint32_t*>(mImageData) +
                   (mInfo.output_scanline * mInfo.output_width);
      }

      MOZ_ASSERT(imageRow, "Should have a row buffer here");

      if (mInfo.out_color_space == MOZ_JCS_EXT_NATIVE_ENDIAN_XRGB) {
        
        if (jpeg_read_scanlines(&mInfo, (JSAMPARRAY)&imageRow, 1) != 1) {
          *suspend = true; 
          break;
        }
        if (mDownscaler) {
          mDownscaler->CommitRow();
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
          
          qcms_transform* transform = gfxPlatform::GetCMSRGBTransform();
          if (transform) {
            qcms_transform_data(transform, sampleRow, sampleRow,
                                mInfo.output_width);
          }
        }
      }

      
      uint32_t idx = mInfo.output_width;

      
      for (; (NS_PTR_TO_UINT32(sampleRow) & 0x3) && idx; --idx) {
        *imageRow++ = gfxPackedPixel(0xFF, sampleRow[0], sampleRow[1],
                                     sampleRow[2]);
        sampleRow += 3;
      }

      
      while (idx >= 4) {
        GFX_BLOCK_RGB_TO_FRGB(sampleRow, imageRow);
        idx       -=  4;
        sampleRow += 12;
        imageRow  +=  4;
      }

      
      while (idx--) {
        
        *imageRow++ = gfxPackedPixel(0xFF, sampleRow[0], sampleRow[1],
                                     sampleRow[2]);
        sampleRow += 3;
      }

      if (mDownscaler) {
        mDownscaler->CommitRow();
      }
  }

  if (top != mInfo.output_scanline) {
    PostInvalidation(nsIntRect(0, top,
                               mInfo.output_width,
                               mInfo.output_scanline - top),
                     mDownscaler ? Some(mDownscaler->TakeInvalidRect())
                                 : Nothing());
  }

  MOZ_ASSERT(!mDownscaler || !mDownscaler->HasInvalidation(),
             "Didn't send downscaler's invalidation");
}



METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  decoder_error_mgr* err = (decoder_error_mgr*) cinfo->err;

  
  nsresult error_code = err->pub.msg_code == JERR_OUT_OF_MEMORY
                      ? NS_ERROR_OUT_OF_MEMORY
                      : NS_ERROR_FAILURE;

#ifdef DEBUG
  char buffer[JMSG_LENGTH_MAX];

  
  (*err->pub.format_message) (cinfo, buffer);

  fprintf(stderr, "JPEG decoding error:\n%s\n", buffer);
#endif

  
  
  longjmp(err->setjmp_buffer, static_cast<int>(error_code));
}









































METHODDEF(void)
init_source (j_decompress_ptr jd)
{
}












METHODDEF(void)
skip_input_data (j_decompress_ptr jd, long num_bytes)
{
  struct jpeg_source_mgr* src = jd->src;
  nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);

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
  struct jpeg_source_mgr* src = jd->src;
  nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);

  if (decoder->mReading) {
    const JOCTET* new_buffer = decoder->mSegment;
    uint32_t new_buflen = decoder->mSegmentLen;

    if (!new_buffer || new_buflen == 0) {
      return false; 
    }

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

  
  const uint32_t new_backtrack_buflen = src->bytes_in_buffer +
                                        decoder->mBackBufferLen;

  
  if (decoder->mBackBufferSize < new_backtrack_buflen) {
    
    
    if (new_backtrack_buflen > MAX_JPEG_MARKER_LENGTH) {
      my_error_exit((j_common_ptr)(&decoder->mInfo));
    }

    
    const size_t roundup_buflen = ((new_backtrack_buflen + 255) >> 8) << 8;
    JOCTET* buf = (JOCTET*)PR_REALLOC(decoder->mBackBuffer, roundup_buflen);
    
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

  
  src->next_input_byte = decoder->mBackBuffer + decoder->mBackBufferLen -
                         decoder->mBackBufferUnreadLen;
  src->bytes_in_buffer += decoder->mBackBufferUnreadLen;
  decoder->mBackBufferLen = (size_t)new_backtrack_buflen;
  decoder->mReading = true;

  return false;
}








METHODDEF(void)
term_source (j_decompress_ptr jd)
{
  nsJPEGDecoder* decoder = (nsJPEGDecoder*)(jd->client_data);

  
  
  MOZ_ASSERT(decoder->mState != JPEG_ERROR,
             "Calling term_source on a JPEG with mState == JPEG_ERROR!");

  
  decoder->NotifyDone();
}

} 
} 








static void cmyk_convert_rgb(JSAMPROW row, JDIMENSION width)
{
  
  JSAMPROW in = row + width*4;
  JSAMPROW out = in;

  for (uint32_t i = width; i > 0; i--) {
    in -= 4;
    out -= 3;

    
    
    

    
    
    
    

    
    
    

    
    
    
    

    
    const uint32_t iC = in[0];
    const uint32_t iM = in[1];
    const uint32_t iY = in[2];
    const uint32_t iK = in[3];
    out[0] = iC*iK/255;   
    out[1] = iM*iK/255;   
    out[2] = iY*iK/255;   
  }
}
