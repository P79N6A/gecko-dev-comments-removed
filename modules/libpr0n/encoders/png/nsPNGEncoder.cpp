







































#include "nsCRT.h"
#include "nsPNGEncoder.h"
#include "prmem.h"
#include "prprf.h"
#include "nsString.h"
#include "nsStreamUtils.h"



#ifdef jmpbuf
#undef jmpbuf
#endif




NS_IMPL_THREADSAFE_ISUPPORTS2(nsPNGEncoder, imgIEncoder, nsIInputStream)

nsPNGEncoder::nsPNGEncoder() : mPNG(nsnull), mPNGinfo(nsnull),
                               mIsAnimation(PR_FALSE),
                               mImageBuffer(nsnull), mImageBufferSize(0),
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
  nsresult rv;

  rv = StartImageEncode(aWidth, aHeight, aInputFormat, aOutputOptions);
  if (!NS_SUCCEEDED(rv))
    return rv;

  rv = AddImageFrame(aData, aLength, aWidth, aHeight, aStride, aInputFormat, aOutputOptions);
  if (!NS_SUCCEEDED(rv))
    return rv;

  rv = EndImageEncode();

  return rv;
}






NS_IMETHODIMP nsPNGEncoder::StartImageEncode(PRUint32 aWidth,
                                             PRUint32 aHeight,
                                             PRUint32 aInputFormat,
                                             const nsAString& aOutputOptions)
{
  PRBool useTransparency = PR_TRUE, skipFirstFrame = PR_FALSE;
  PRUint32 numFrames = 1;
  PRUint32 numPlays = 0; 

  
  if (mImageBuffer != nsnull)
    return NS_ERROR_ALREADY_INITIALIZED;

  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB)
    return NS_ERROR_INVALID_ARG;

  
  nsresult rv = ParseOptions(aOutputOptions, &useTransparency, &skipFirstFrame,
                             &numFrames, &numPlays, nsnull, nsnull,
                             nsnull, nsnull, nsnull);
  if (rv != NS_OK) { return rv; }

  if (numFrames > 1) {
    mIsAnimation = PR_TRUE;
  }

  
  mPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                 png_voidp_NULL,
                                 ErrorCallback,
                                 ErrorCallback);
  if (! mPNG)
    return NS_ERROR_OUT_OF_MEMORY;

  mPNGinfo = png_create_info_struct(mPNG);
  if (! mPNGinfo) {
    png_destroy_write_struct(&mPNG, nsnull);
    return NS_ERROR_FAILURE;
  }

  
  
  
  if (setjmp(png_jmpbuf(mPNG))) {
    png_destroy_write_struct(&mPNG, &mPNGinfo);
    return NS_ERROR_FAILURE;
  }

  
  
  
  mImageBufferSize = 8192;
  mImageBuffer = (PRUint8*)PR_Malloc(mImageBufferSize);
  if (!mImageBuffer) {
    png_destroy_write_struct(&mPNG, &mPNGinfo);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mImageBufferUsed = 0;

  
  png_set_write_fn(mPNG, this, WriteCallback, NULL);

  
  int colorType;
  if ((aInputFormat == INPUT_FORMAT_HOSTARGB ||
       aInputFormat == INPUT_FORMAT_RGBA)  &&  useTransparency)
    colorType = PNG_COLOR_TYPE_RGB_ALPHA;
  else
    colorType = PNG_COLOR_TYPE_RGB;

  png_set_IHDR(mPNG, mPNGinfo, aWidth, aHeight, 8, colorType,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  if (mIsAnimation) {
    png_set_first_frame_is_hidden(mPNG, mPNGinfo, skipFirstFrame);
    png_set_acTL(mPNG, mPNGinfo, numFrames, numPlays);
  }

  

  png_write_info(mPNG, mPNGinfo);

  return NS_OK;
}


NS_IMETHODIMP nsPNGEncoder::AddImageFrame(const PRUint8* aData,
                                          PRUint32 aLength, 
                                          PRUint32 aWidth,
                                          PRUint32 aHeight,
                                          PRUint32 aStride,
                                          PRUint32 aInputFormat,
                                          const nsAString& aFrameOptions)
{
  PRBool useTransparency= PR_TRUE;
  PRUint32 delay_ms = 500;
  PRUint32 dispose_op = PNG_DISPOSE_OP_NONE;
  PRUint32 blend_op = PNG_BLEND_OP_SOURCE;
  PRUint32 x_offset = 0, y_offset = 0;

  
  if (mImageBuffer == nsnull)
    return NS_ERROR_NOT_INITIALIZED;

  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB)
    return NS_ERROR_INVALID_ARG;

  
  if (setjmp(png_jmpbuf(mPNG))) {
    png_destroy_write_struct(&mPNG, &mPNGinfo);
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = ParseOptions(aFrameOptions, &useTransparency, nsnull,
                             nsnull, nsnull, &dispose_op, &blend_op,
                             &delay_ms, &x_offset, &y_offset);
  if (rv != NS_OK) { return rv; }

  if (mIsAnimation) {
    
    png_write_frame_head(mPNG, mPNGinfo, nsnull,
                         aWidth, aHeight, x_offset, y_offset,
                         delay_ms, 1000, dispose_op, blend_op);
  }

  
  
  if ((aInputFormat == INPUT_FORMAT_RGB &&
       aStride < aWidth * 3) ||
      ((aInputFormat == INPUT_FORMAT_RGBA || aInputFormat == INPUT_FORMAT_HOSTARGB) &&
       aStride < aWidth * 4)) {
    NS_WARNING("Invalid stride for InitFromData/AddImageFrame");
    return NS_ERROR_INVALID_ARG;
  }

  
  
  if (aInputFormat == INPUT_FORMAT_HOSTARGB) {
    
    PRUint8* row = new PRUint8[aWidth * 4];
    for (PRUint32 y = 0; y < aHeight; y ++) {
      ConvertHostARGBRow(&aData[y * aStride], row, aWidth, useTransparency);
      png_write_row(mPNG, row);
    }
    delete[] row;

  } else if (aInputFormat == INPUT_FORMAT_RGBA && ! useTransparency) {
    
    PRUint8* row = new PRUint8[aWidth * 4];
    for (PRUint32 y = 0; y < aHeight; y ++) {
      StripAlpha(&aData[y * aStride], row, aWidth);
      png_write_row(mPNG, row);
    }
    delete[] row;

  } else if (aInputFormat == INPUT_FORMAT_RGB ||
             aInputFormat == INPUT_FORMAT_RGBA) {
    
    for (PRUint32 y = 0; y < aHeight; y ++) {
      png_write_row(mPNG, (PRUint8*)&aData[y * aStride]);
    }

  } else {
    NS_NOTREACHED("Bad format type");
    return NS_ERROR_INVALID_ARG;
  }

  if (mIsAnimation) {
    png_write_frame_tail(mPNG, mPNGinfo);
  }

  return NS_OK;
}


NS_IMETHODIMP nsPNGEncoder::EndImageEncode()
{
  
  if (mImageBuffer == nsnull)
    return NS_ERROR_NOT_INITIALIZED;

  
  if (setjmp(png_jmpbuf(mPNG))) {
    png_destroy_write_struct(&mPNG, &mPNGinfo);
    return NS_ERROR_FAILURE;
  }

  png_write_end(mPNG, mPNGinfo);
  png_destroy_write_struct(&mPNG, &mPNGinfo);

  
  if (!mImageBuffer)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


nsresult
nsPNGEncoder::ParseOptions(const nsAString& aOptions,
                           PRBool* useTransparency,
                           PRBool* skipFirstFrame,
                           PRUint32* numFrames,
                           PRUint32* numPlays,
                           PRUint32* frameDispose,
                           PRUint32* frameBlend,
                           PRUint32* frameDelay,
                           PRUint32* offsetX,
                           PRUint32* offsetY)
{
  char* token;
  char* options = nsCRT::strdup(PromiseFlatCString(NS_ConvertUTF16toUTF8(aOptions)).get());

  while ((token = nsCRT::strtok(options, ";", &options))) {
    
    char* equals = token, *value = nsnull;
    while(*equals != '=' && *equals) { ++equals; }
    if (*equals == '=') { value = equals + 1; }

    if (value) { *equals = '\0'; } 

    
    if (nsCRT::strcmp(token, "transparency") == 0 && useTransparency) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (nsCRT::strcmp(value, "none") == 0 || nsCRT::strcmp(value, "no") == 0) {
        *useTransparency = PR_FALSE;
      } else if (nsCRT::strcmp(value, "yes") == 0) {
        *useTransparency = PR_TRUE;
      } else {
        return NS_ERROR_INVALID_ARG;
      }

    
    } else if (nsCRT::strcmp(token, "skipfirstframe") == 0 && skipFirstFrame) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (nsCRT::strcmp(value, "no") == 0) {
        *skipFirstFrame = PR_FALSE;
      } else if (nsCRT::strcmp(value, "yes") == 0) {
        *skipFirstFrame = PR_TRUE;
      } else {
        return NS_ERROR_INVALID_ARG;
      }

    
    } else if (nsCRT::strcmp(token, "frames") == 0 && numFrames) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (PR_sscanf(value, "%u", numFrames) != 1) { return NS_ERROR_INVALID_ARG; }

      
      if (*numFrames == 0) { return NS_ERROR_INVALID_ARG; }

    
    } else if (nsCRT::strcmp(token, "plays") == 0 && numPlays) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      
      if (PR_sscanf(value, "%u", numPlays) != 1) { return NS_ERROR_INVALID_ARG; }

    
    } else if (nsCRT::strcmp(token, "dispose") == 0 && frameDispose) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (nsCRT::strcmp(value, "none") == 0) {
        *frameDispose = PNG_DISPOSE_OP_NONE;
      } else if (nsCRT::strcmp(value, "background") == 0) {
        *frameDispose = PNG_DISPOSE_OP_BACKGROUND;
      } else if (nsCRT::strcmp(value, "previous") == 0) {
        *frameDispose = PNG_DISPOSE_OP_PREVIOUS;
      } else {
        return NS_ERROR_INVALID_ARG;
      }

    
    } else if (nsCRT::strcmp(token, "blend") == 0 && frameBlend) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (nsCRT::strcmp(value, "source") == 0) {
        *frameBlend = PNG_BLEND_OP_SOURCE;
      } else if (nsCRT::strcmp(value, "over") == 0) {
        *frameBlend = PNG_BLEND_OP_OVER;
      } else {
        return NS_ERROR_INVALID_ARG;
      }

    
    } else if (nsCRT::strcmp(token, "delay") == 0 && frameDelay) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (PR_sscanf(value, "%u", frameDelay) != 1) { return NS_ERROR_INVALID_ARG; }

    
    } else if (nsCRT::strcmp(token, "xoffset") == 0 && offsetX) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (PR_sscanf(value, "%u", offsetX) != 1) { return NS_ERROR_INVALID_ARG; }

    
    } else if (nsCRT::strcmp(token, "yoffset") == 0 && offsetY) {
      if (!value) { return NS_ERROR_INVALID_ARG; }

      if (PR_sscanf(value, "%u", offsetY) != 1) { return NS_ERROR_INVALID_ARG; }

    
    } else {
      return NS_ERROR_INVALID_ARG;
    }

    if (value) { *equals = '='; } 
  }

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
nsPNGEncoder::ErrorCallback(png_structp png_ptr, png_const_charp warning_msg)
{
#ifdef DEBUG
	
	PR_fprintf(PR_STDERR, "PNG Encoder: %s\n", warning_msg);;
#endif
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
