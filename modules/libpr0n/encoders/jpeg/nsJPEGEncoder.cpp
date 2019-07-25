





































#include "nsJPEGEncoder.h"
#include "prmem.h"
#include "prprf.h"
#include "nsString.h"
#include "nsStreamUtils.h"

#include <setjmp.h>
#include "jerror.h"

using namespace mozilla;

NS_IMPL_THREADSAFE_ISUPPORTS3(nsJPEGEncoder, imgIEncoder, nsIInputStream, nsIAsyncInputStream)


struct encoder_error_mgr {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

nsJPEGEncoder::nsJPEGEncoder() : mFinished(PR_FALSE),
                                 mImageBuffer(nsnull), mImageBufferSize(0),
                                 mImageBufferUsed(0), mImageBufferReadPoint(0),
                                 mCallback(nsnull),
                                 mCallbackTarget(nsnull), mNotifyThreshold(0),
                                 mReentrantMonitor("nsJPEGEncoder.mReentrantMonitor")
{
}

nsJPEGEncoder::~nsJPEGEncoder()
{
  if (mImageBuffer) {
    PR_Free(mImageBuffer);
    mImageBuffer = nsnull;
  }
}









NS_IMETHODIMP nsJPEGEncoder::InitFromData(const PRUint8* aData,
                                          PRUint32 aLength, 
                                          PRUint32 aWidth,
                                          PRUint32 aHeight,
                                          PRUint32 aStride,
                                          PRUint32 aInputFormat,
                                          const nsAString& aOutputOptions)
{
  
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

  
  if (mImageBuffer != nsnull)
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
      const PRUint8* row = &aData[cinfo.next_scanline * aStride];
      jpeg_write_scanlines(&cinfo, const_cast<PRUint8**>(&row), 1);
    }
  } else if (aInputFormat == INPUT_FORMAT_RGBA) {
    PRUint8* row = new PRUint8[aWidth * 3];
    while (cinfo.next_scanline < cinfo.image_height) {
      StripAlpha(&aData[cinfo.next_scanline * aStride], row, aWidth);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }
    delete[] row;
  } else if (aInputFormat == INPUT_FORMAT_HOSTARGB) {
    PRUint8* row = new PRUint8[aWidth * 3];
    while (cinfo.next_scanline < cinfo.image_height) {
      ConvertHostARGBRow(&aData[cinfo.next_scanline * aStride], row, aWidth);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }
    delete[] row;
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  mFinished = PR_TRUE;
  NotifyListener();

  
  if (!mImageBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::StartImageEncode(PRUint32 aWidth,
                                              PRUint32 aHeight,
                                              PRUint32 aInputFormat,
                                              const nsAString& aOutputOptions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsJPEGEncoder::AddImageFrame(const PRUint8* aData,
                                           PRUint32 aLength,
                                           PRUint32 aWidth,
                                           PRUint32 aHeight,
                                           PRUint32 aStride,
                                           PRUint32 aFrameFormat,
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
  if (mImageBuffer != nsnull) {
    PR_Free(mImageBuffer);
    mImageBuffer = nsnull;
    mImageBufferSize = 0;
    mImageBufferUsed = 0;
    mImageBufferReadPoint = 0;
  }
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::Available(PRUint32 *_retval)
{
  if (!mImageBuffer)
    return NS_BASE_STREAM_CLOSED;

  *_retval = mImageBufferUsed - mImageBufferReadPoint;
  return NS_OK;
}


NS_IMETHODIMP nsJPEGEncoder::Read(char * aBuf, PRUint32 aCount,
                                  PRUint32 *_retval)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, _retval);
}


NS_IMETHODIMP nsJPEGEncoder::ReadSegments(nsWriteSegmentFun aWriter, void *aClosure, PRUint32 aCount, PRUint32 *_retval)
{
  
  ReentrantMonitorAutoEnter autoEnter(mReentrantMonitor);

  PRUint32 maxCount = mImageBufferUsed - mImageBufferReadPoint;
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


NS_IMETHODIMP nsJPEGEncoder::IsNonBlocking(PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsJPEGEncoder::AsyncWait(nsIInputStreamCallback *aCallback,
                                       PRUint32 aFlags,
                                       PRUint32 aRequestedCount,
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
nsJPEGEncoder::ConvertHostARGBRow(const PRUint8* aSrc, PRUint8* aDest,
                                 PRUint32 aPixelWidth)
{
  for (PRUint32 x = 0; x < aPixelWidth; x ++) {
    const PRUint32& pixelIn = ((const PRUint32*)(aSrc))[x];
    PRUint8 *pixelOut = &aDest[x * 3];

    PRUint8 alpha = (pixelIn & 0xff000000) >> 24;
    if (alpha == 0) {
      pixelOut[0] = pixelOut[1] = pixelOut[2] = 0;
    } else {
      pixelOut[0] = (((pixelIn & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
      pixelOut[1] = (((pixelIn & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
      pixelOut[2] = (((pixelIn & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
    }
  }
}






void
nsJPEGEncoder::StripAlpha(const PRUint8* aSrc, PRUint8* aDest,
                          PRUint32 aPixelWidth)
{
  for (PRUint32 x = 0; x < aPixelWidth; x ++) {
    const PRUint8* pixelIn = &aSrc[x * 4];
    PRUint8* pixelOut = &aDest[x * 3];
    pixelOut[0] = pixelIn[0];
    pixelOut[1] = pixelIn[1];
    pixelOut[2] = pixelIn[2];
  }
}








void 
nsJPEGEncoder::initDestination(jpeg_compress_struct* cinfo)
{
  nsJPEGEncoder* that = static_cast<nsJPEGEncoder*>(cinfo->client_data);
  NS_ASSERTION(! that->mImageBuffer, "Image buffer already initialized");

  that->mImageBufferSize = 8192;
  that->mImageBuffer = (PRUint8*)PR_Malloc(that->mImageBufferSize);
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

  PRUint8* newBuf = (PRUint8*)PR_Realloc(that->mImageBuffer,
                                         that->mImageBufferSize);
  if (! newBuf) {
    
    PR_Free(that->mImageBuffer);
    that->mImageBuffer = nsnull;
    that->mImageBufferSize = 0;
    that->mImageBufferUsed = 0;

    
    longjmp(((encoder_error_mgr*)(cinfo->err))->setjmp_buffer,
            NS_ERROR_OUT_OF_MEMORY);
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

  
  longjmp(err->setjmp_buffer, error_code);
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
    
    
    mCallback = nsnull;
    mCallbackTarget = nsnull;
    mNotifyThreshold = 0;

    callback->OnInputStreamReady(this);
  }
}
