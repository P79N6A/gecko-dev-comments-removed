




#include "nsJPEGEncoder.h"
#include "prmem.h"
#include "prprf.h"
#include "nsString.h"
#include "nsStreamUtils.h"
#include "gfxColor.h"

#include <setjmp.h>
#include "jerror.h"

using namespace mozilla;

NS_IMPL_THREADSAFE_ISUPPORTS3(nsJPEGEncoder, imgIEncoder, nsIInputStream, nsIAsyncInputStream)


struct encoder_error_mgr {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

nsJPEGEncoder::nsJPEGEncoder() : mFinished(false),
                                 mImageBuffer(nullptr), mImageBufferSize(0),
                                 mImageBufferUsed(0), mImageBufferReadPoint(0),
                                 mCallback(nullptr),
                                 mCallbackTarget(nullptr), mNotifyThreshold(0),
                                 mReentrantMonitor("nsJPEGEncoder.mReentrantMonitor")
{
}

nsJPEGEncoder::~nsJPEGEncoder()
{
  if (mImageBuffer) {
    PR_Free(mImageBuffer);
    mImageBuffer = nullptr;
  }
}









NS_IMETHODIMP nsJPEGEncoder::InitFromData(const uint8_t* aData,
                                          uint32_t aLength, 
                                          uint32_t aWidth,
                                          uint32_t aHeight,
                                          uint32_t aStride,
                                          uint32_t aInputFormat,
                                          const nsAString& aOutputOptions)
{
  NS_ENSURE_ARG(aData);

  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB)
    return NS_ERROR_INVALID_ARG;

  
  
  if ((aInputFormat == INPUT_FORMAT_RGB &&
       aStride < aWidth * 3) ||
      ((aInputFormat == INPUT_FORMAT_RGBA || aInputFormat == INPUT_FORMAT_HOSTARGB) &&
       aStride < aWidth * 4)) {
    NS_WARNING("Invalid stride for InitFromData");
    return NS_ERROR_INVALID_ARG;
  }

  
  if (mImageBuffer != nullptr)
    return NS_ERROR_ALREADY_INITIALIZED;

  
  int quality = 92;
  if (aOutputOptions.Length() > 0) {
    
    const nsString qualityPrefix(NS_LITERAL_STRING("quality="));
    if (aOutputOptions.Length() > qualityPrefix.Length()  &&
        StringBeginsWith(aOutputOptions, qualityPrefix)) {
      
      nsCString value = NS_ConvertUTF16toUTF8(Substring(aOutputOptions,
                                                        qualityPrefix.Length()));
      int newquality = -1;
      if (PR_sscanf(value.get(), "%d", &newquality) == 1) {
        if (newquality >= 0 && newquality <= 100) {
          quality = newquality;
        } else {
          NS_WARNING("Quality value out of range, should be 0-100, using default");
        }
      } else {
        NS_WARNING("Quality value invalid, should be integer 0-100, using default");
      }
    }
    else {
      return NS_ERROR_INVALID_ARG;
    }
  }

  jpeg_compress_struct cinfo;

  
  
  encoder_error_mgr errmgr;
  cinfo.err = jpeg_std_error(&errmgr.pub);
  errmgr.pub.error_exit = errorExit;
  
  if (setjmp(errmgr.setjmp_buffer)) {
    
    
    return NS_ERROR_FAILURE;
  }

  jpeg_create_compress(&cinfo);
  cinfo.image_width = aWidth;
  cinfo.image_height = aHeight;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  cinfo.data_precision = 8;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, 1); 
  if (quality >= 90) {
    int i;
    for (i=0; i < MAX_COMPONENTS; i++) {
      cinfo.comp_info[i].h_samp_factor=1;
      cinfo.comp_info[i].v_samp_factor=1;
    }
  }

  
  jpeg_destination_mgr destmgr;
  destmgr.init_destination = initDestination;
  destmgr.empty_output_buffer = emptyOutputBuffer;
  destmgr.term_destination = termDestination;
  cinfo.dest = &destmgr;
  cinfo.client_data = this;

  jpeg_start_compress(&cinfo, 1);

  
  if (aInputFormat == INPUT_FORMAT_RGB) {
    while (cinfo.next_scanline < cinfo.image_height) {
      const uint8_t* row = &aData[cinfo.next_scanline * aStride];
      jpeg_write_scanlines(&cinfo, const_cast<uint8_t**>(&row), 1);
    }
  } else if (aInputFormat == INPUT_FORMAT_RGBA) {
    uint8_t* row = new uint8_t[aWidth * 3];
    while (cinfo.next_scanline < cinfo.image_height) {
      ConvertRGBARow(&aData[cinfo.next_scanline * aStride], row, aWidth);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }
    delete[] row;
  } else if (aInputFormat == INPUT_FORMAT_HOSTARGB) {
    uint8_t* row = new uint8_t[aWidth * 3];
    while (cinfo.next_scanline < cinfo.image_height) {
      ConvertHostARGBRow(&aData[cinfo.next_scanline * aStride], row, aWidth);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }
    delete[] row;
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  mFinished = true;
  NotifyListener();

  
  if (!mImageBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::StartImageEncode(uint32_t aWidth,
                                              uint32_t aHeight,
                                              uint32_t aInputFormat,
                                              const nsAString& aOutputOptions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsJPEGEncoder::GetImageBufferUsed(uint32_t *aOutputSize)
{
  NS_ENSURE_ARG_POINTER(aOutputSize);
  *aOutputSize = mImageBufferUsed;
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::GetImageBuffer(char **aOutputBuffer)
{
  NS_ENSURE_ARG_POINTER(aOutputBuffer);
  *aOutputBuffer = reinterpret_cast<char*>(mImageBuffer);
  return NS_OK;
}

NS_IMETHODIMP nsJPEGEncoder::AddImageFrame(const uint8_t* aData,
                                           uint32_t aLength,
                                           uint32_t aWidth,
                                           uint32_t aHeight,
                                           uint32_t aStride,
                                           uint32_t aFrameFormat,
                                           const nsAString& aFrameOptions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsJPEGEncoder::EndImageEncode()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP nsJPEGEncoder::Close()
{
  if (mImageBuffer != nullptr) {
    PR_Free(mImageBuffer);
    mImageBuffer = nullptr;
    mImageBufferSize = 0;
    mImageBufferUsed = 0;
    mImageBufferReadPoint = 0;
  }
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::Available(uint64_t *_retval)
{
  if (!mImageBuffer)
    return NS_BASE_STREAM_CLOSED;

  *_retval = mImageBufferUsed - mImageBufferReadPoint;
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::Read(char * aBuf, uint32_t aCount,
                                  uint32_t *_retval)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, _retval);
}


NS_IMETHODIMP nsJPEGEncoder::ReadSegments(nsWriteSegmentFun aWriter, void *aClosure, uint32_t aCount, uint32_t *_retval)
{
  
  ReentrantMonitorAutoEnter autoEnter(mReentrantMonitor);

  uint32_t maxCount = mImageBufferUsed - mImageBufferReadPoint;
  if (maxCount == 0) {
    *_retval = 0;
    return mFinished ? NS_OK : NS_BASE_STREAM_WOULD_BLOCK;
  }

  if (aCount > maxCount)
    aCount = maxCount;
  nsresult rv = aWriter(this, aClosure,
                        reinterpret_cast<const char*>(mImageBuffer+mImageBufferReadPoint),
                        0, aCount, _retval);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(*_retval <= aCount, "bad write count");
    mImageBufferReadPoint += *_retval;
  }

  
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::IsNonBlocking(bool *_retval)
{
  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP nsJPEGEncoder::AsyncWait(nsIInputStreamCallback *aCallback,
                                       uint32_t aFlags,
                                       uint32_t aRequestedCount,
                                       nsIEventTarget *aTarget)
{
  if (aFlags != 0)
    return NS_ERROR_NOT_IMPLEMENTED;

  if (mCallback || mCallbackTarget)
    return NS_ERROR_UNEXPECTED;

  mCallbackTarget = aTarget;
  
  mNotifyThreshold = aRequestedCount;
  if (!aRequestedCount)
    mNotifyThreshold = 1024; 

  
  
  
  
  mCallback = aCallback;

  
  NotifyListener();
  return NS_OK;
}

NS_IMETHODIMP nsJPEGEncoder::CloseWithStatus(nsresult aStatus)
{
  return Close();
}









void
nsJPEGEncoder::ConvertHostARGBRow(const uint8_t* aSrc, uint8_t* aDest,
                                  uint32_t aPixelWidth)
{
  for (uint32_t x = 0; x < aPixelWidth; x++) {
    const uint32_t& pixelIn = ((const uint32_t*)(aSrc))[x];
    uint8_t *pixelOut = &aDest[x * 3];

    pixelOut[0] = (pixelIn & 0xff0000) >> 16;
    pixelOut[1] = (pixelIn & 0x00ff00) >>  8;
    pixelOut[2] = (pixelIn & 0x0000ff) >>  0;
  }
}






void
nsJPEGEncoder::ConvertRGBARow(const uint8_t* aSrc, uint8_t* aDest,
                              uint32_t aPixelWidth)
{
  for (uint32_t x = 0; x < aPixelWidth; x++) {
    const uint8_t* pixelIn = &aSrc[x * 4];
    uint8_t* pixelOut = &aDest[x * 3];

    uint8_t alpha = pixelIn[3];
    pixelOut[0] = gfxPreMultiply(pixelIn[0], alpha);
    pixelOut[1] = gfxPreMultiply(pixelIn[1], alpha);
    pixelOut[2] = gfxPreMultiply(pixelIn[2], alpha);
  }
}







void 
nsJPEGEncoder::initDestination(jpeg_compress_struct* cinfo)
{
  nsJPEGEncoder* that = static_cast<nsJPEGEncoder*>(cinfo->client_data);
  NS_ASSERTION(! that->mImageBuffer, "Image buffer already initialized");

  that->mImageBufferSize = 8192;
  that->mImageBuffer = (uint8_t*)PR_Malloc(that->mImageBufferSize);
  that->mImageBufferUsed = 0;

  cinfo->dest->next_output_byte = that->mImageBuffer;
  cinfo->dest->free_in_buffer = that->mImageBufferSize;
}













boolean 
nsJPEGEncoder::emptyOutputBuffer(jpeg_compress_struct* cinfo)
{
  nsJPEGEncoder* that = static_cast<nsJPEGEncoder*>(cinfo->client_data);
  NS_ASSERTION(that->mImageBuffer, "No buffer to empty!");

  
  
  ReentrantMonitorAutoEnter autoEnter(that->mReentrantMonitor);

  that->mImageBufferUsed = that->mImageBufferSize;

  
  that->mImageBufferSize *= 2;

  uint8_t* newBuf = (uint8_t*)PR_Realloc(that->mImageBuffer,
                                         that->mImageBufferSize);
  if (! newBuf) {
    
    PR_Free(that->mImageBuffer);
    that->mImageBuffer = nullptr;
    that->mImageBufferSize = 0;
    that->mImageBufferUsed = 0;

    
    
    
    longjmp(((encoder_error_mgr*)(cinfo->err))->setjmp_buffer,
            static_cast<int>(NS_ERROR_OUT_OF_MEMORY));
  }
  that->mImageBuffer = newBuf;

  cinfo->dest->next_output_byte = &that->mImageBuffer[that->mImageBufferUsed];
  cinfo->dest->free_in_buffer = that->mImageBufferSize - that->mImageBufferUsed;
  return 1;
}









void 
nsJPEGEncoder::termDestination(jpeg_compress_struct* cinfo)
{
  nsJPEGEncoder* that = static_cast<nsJPEGEncoder*>(cinfo->client_data);
  if (! that->mImageBuffer)
    return;
  that->mImageBufferUsed = cinfo->dest->next_output_byte - that->mImageBuffer;
  NS_ASSERTION(that->mImageBufferUsed < that->mImageBufferSize,
               "JPEG library busted, got a bad image buffer size");
  that->NotifyListener();
}







void 
nsJPEGEncoder::errorExit(jpeg_common_struct* cinfo)
{
  nsresult error_code;
  encoder_error_mgr *err = (encoder_error_mgr *) cinfo->err;

  
  switch (cinfo->err->msg_code) {
    case JERR_OUT_OF_MEMORY:
      error_code = NS_ERROR_OUT_OF_MEMORY;
      break;
    default:
      error_code = NS_ERROR_FAILURE;
  }

  
  
  longjmp(err->setjmp_buffer, static_cast<int>(error_code));
}

void
nsJPEGEncoder::NotifyListener()
{
  
  
  
  
  ReentrantMonitorAutoEnter autoEnter(mReentrantMonitor);

  if (mCallback &&
      (mImageBufferUsed - mImageBufferReadPoint >= mNotifyThreshold ||
       mFinished)) {
    nsCOMPtr<nsIInputStreamCallback> callback;
    if (mCallbackTarget) {
      NS_NewInputStreamReadyEvent(getter_AddRefs(callback),
                                  mCallback,
                                  mCallbackTarget);
    } else {
      callback = mCallback;
    }

    NS_ASSERTION(callback, "Shouldn't fail to make the callback");
    
    
    mCallback = nullptr;
    mCallbackTarget = nullptr;
    mNotifyThreshold = 0;

    callback->OnInputStreamReady(this);
  }
}
