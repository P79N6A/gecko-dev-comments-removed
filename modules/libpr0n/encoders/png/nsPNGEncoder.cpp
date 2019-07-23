






































#include "nsPNGEncoder.h"
#include "prmem.h"
#include "nsString.h"
#include "nsStreamUtils.h"



#ifdef jmpbuf
#undef jmpbuf
#endif




NS_IMPL_THREADSAFE_ISUPPORTS2(nsPNGEncoder, imgIEncoder, nsIInputStream)

nsPNGEncoder::nsPNGEncoder() : mImageBuffer(nsnull), mImageBufferSize(0),
                                 mImageBufferUsed(0), mImageBufferReadPoint(0)
{
}

nsPNGEncoder::~nsPNGEncoder()
{
  if (mImageBuffer) {
    PR_Free(mImageBuffer);
    mImageBuffer = nsnull;
  }
}









NS_IMETHODIMP nsPNGEncoder::InitFromData(const PRUint8* aData,
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

  
  PRBool useTransparency = PR_TRUE;
  if (aOutputOptions.Length() >= 17) {
    if (StringBeginsWith(aOutputOptions,  NS_LITERAL_STRING("transparency=none")))
      useTransparency = PR_FALSE;
  }

  
  png_struct* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                png_voidp_NULL,
                                                png_error_ptr_NULL,
                                                png_error_ptr_NULL);
  if (! png_ptr)
    return NS_ERROR_OUT_OF_MEMORY;
  png_info* info_ptr = png_create_info_struct(png_ptr);
  if (! info_ptr)
  {
    png_destroy_write_struct(&png_ptr, nsnull);
    return NS_ERROR_FAILURE;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  mImageBufferSize = 8192;
  mImageBuffer = (PRUint8*)PR_Malloc(mImageBufferSize);
  if (!mImageBuffer) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mImageBufferUsed = 0;

  
  png_set_write_fn(png_ptr, this, WriteCallback, NULL);

  
  int colorType;
  if ((aInputFormat == INPUT_FORMAT_HOSTARGB ||
       aInputFormat == INPUT_FORMAT_RGBA)  &&  useTransparency)
    colorType = PNG_COLOR_TYPE_RGB_ALPHA;
  else
    colorType = PNG_COLOR_TYPE_RGB;

  png_set_IHDR(png_ptr, info_ptr, aWidth, aHeight, 8, colorType,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  
  
  if (aInputFormat == INPUT_FORMAT_HOSTARGB) {
    
    PRUint8* row = new PRUint8[aWidth * 4];
    for (PRUint32 y = 0; y < aHeight; y ++) {
      ConvertHostARGBRow(&aData[y * aStride], row, aWidth, useTransparency);
      png_write_row(png_ptr, row);
    }
    delete[] row;

  } else if (aInputFormat == INPUT_FORMAT_RGBA && ! useTransparency) {
    
    PRUint8* row = new PRUint8[aWidth * 4];
    for (PRUint32 y = 0; y < aHeight; y ++) {
      StripAlpha(&aData[y * aStride], row, aWidth);
      png_write_row(png_ptr, row);
    }
    delete[] row;

  } else if (aInputFormat == INPUT_FORMAT_RGB ||
             aInputFormat == INPUT_FORMAT_RGBA) {
    
    for (PRUint32 y = 0; y < aHeight; y ++) {
      png_write_row(png_ptr, (PRUint8*)&aData[y * aStride]);
    }

  } else {
    NS_NOTREACHED("Bad format type");
  }

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  
  if (!mImageBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}



NS_IMETHODIMP nsPNGEncoder::Close()
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


NS_IMETHODIMP nsPNGEncoder::Available(PRUint32 *_retval)
{
  if (!mImageBuffer)
    return NS_BASE_STREAM_CLOSED;

  *_retval = mImageBufferUsed - mImageBufferReadPoint;
  return NS_OK;
}


NS_IMETHODIMP nsPNGEncoder::Read(char * aBuf, PRUint32 aCount,
                                 PRUint32 *_retval)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, _retval);
}


NS_IMETHODIMP nsPNGEncoder::ReadSegments(nsWriteSegmentFun aWriter,
                                         void *aClosure, PRUint32 aCount,
                                         PRUint32 *_retval)
{
  PRUint32 maxCount = mImageBufferUsed - mImageBufferReadPoint;
  if (maxCount == 0) {
    *_retval = 0;
    return NS_OK;
  }

  if (aCount > maxCount)
    aCount = maxCount;
  nsresult rv = aWriter(this, aClosure,
                        NS_REINTERPRET_CAST(const char*, mImageBuffer),
                        0, aCount, _retval);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(*_retval <= aCount, "bad write count");
    mImageBufferReadPoint += *_retval;
  }

  
  return NS_OK;
}


NS_IMETHODIMP nsPNGEncoder::IsNonBlocking(PRBool *_retval)
{
  *_retval = PR_FALSE;  
  return NS_OK;
}









void
nsPNGEncoder::ConvertHostARGBRow(const PRUint8* aSrc, PRUint8* aDest,
                                  PRUint32 aPixelWidth, PRBool aUseTransparency)
{
  PRUint32 pixelStride = aUseTransparency ? 4 : 3;
  for (PRUint32 x = 0; x < aPixelWidth; x ++) {
    const PRUint32& pixelIn = ((const PRUint32*)(aSrc))[x];
    PRUint8 *pixelOut = &aDest[x * pixelStride];

    PRUint8 alpha = (pixelIn & 0xff000000) >> 24;
    if (alpha == 0) {
      pixelOut[0] = pixelOut[1] = pixelOut[2] = pixelOut[3] = 0;
    } else {
      pixelOut[0] = (((pixelIn & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
      pixelOut[1] = (((pixelIn & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
      pixelOut[2] = (((pixelIn & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
      if (aUseTransparency)
        pixelOut[3] = alpha;
    }
  }
}






void
nsPNGEncoder::StripAlpha(const PRUint8* aSrc, PRUint8* aDest,
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
nsPNGEncoder::WriteCallback(png_structp png, png_bytep data, png_size_t size)
{
  nsPNGEncoder* that = NS_STATIC_CAST(nsPNGEncoder*, png_get_io_ptr(png));
  if (! that->mImageBuffer)
    return;

  if (that->mImageBufferUsed + size > that->mImageBufferSize) {
    
    that->mImageBufferSize *= 2;
    PRUint8* newBuf = (PRUint8*)PR_Realloc(that->mImageBuffer,
                                           that->mImageBufferSize);
    if (! newBuf) {
      
      PR_Free(that->mImageBuffer);
      that->mImageBufferSize = 0;
      that->mImageBufferUsed = 0;
      return;
    }
    that->mImageBuffer = newBuf;
  }
  memcpy(&that->mImageBuffer[that->mImageBufferUsed], data, size);
  that->mImageBufferUsed += size;
}
