



 
 
#include "nsITransferable.h"
#include "nsImageClipboard.h"
#include "nsGfxCIID.h"
#include "nsMemory.h"
#include "prmem.h"
#include "imgIEncoder.h"
#include "nsLiteralString.h"
#include "nsComponentManagerUtils.h"

#define BFH_LENGTH 14














nsImageToClipboard::nsImageToClipboard(imgIContainer* aInImage, bool aWantDIBV5)
  : mImage(aInImage)
  , mWantDIBV5(aWantDIBV5)
{
  
}








nsImageToClipboard::~nsImageToClipboard()
{
}










nsresult
nsImageToClipboard :: GetPicture ( HANDLE* outBits )
{
  NS_ASSERTION ( outBits, "Bad parameter" );

  return CreateFromImage ( mImage, outBits );

} 







int32_t 
nsImageToClipboard :: CalcSize ( int32_t aHeight, int32_t aColors, WORD aBitsPerPixel, int32_t aSpanBytes )
{
  int32_t HeaderMem = sizeof(BITMAPINFOHEADER);

  
  if (aBitsPerPixel < 16)
    HeaderMem += aColors * sizeof(RGBQUAD);

  if (aHeight < 0)
    aHeight = -aHeight;

  return (HeaderMem + (aHeight * aSpanBytes));
}







int32_t 
nsImageToClipboard::CalcSpanLength(uint32_t aWidth, uint32_t aBitCount)
{
  int32_t spanBytes = (aWidth * aBitCount) >> 5;
  
  if ((aWidth * aBitCount) & 0x1F)
    spanBytes++;
  spanBytes <<= 2;

  return spanBytes;
}








nsresult
nsImageToClipboard::CreateFromImage ( imgIContainer* inImage, HANDLE* outBitmap )
{
    nsresult rv;
    *outBitmap = nullptr;

    nsRefPtr<gfxASurface> surface;
    inImage->GetFrame(imgIContainer::FRAME_CURRENT,
                      imgIContainer::FLAG_SYNC_DECODE,
                      getter_AddRefs(surface));
    NS_ENSURE_TRUE(surface, NS_ERROR_FAILURE);

    nsRefPtr<gfxImageSurface> frame(surface->GetAsReadableARGB32ImageSurface());
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance("@mozilla.org/image/encoder;2?type=image/bmp", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    uint32_t format;
    nsAutoString options;
    if (mWantDIBV5) {
      options.AppendASCII("version=5;bpp=");
    } else {
      options.AppendASCII("version=3;bpp=");
    }
    switch (frame->Format()) {
    case gfxASurface::ImageFormatARGB32:
        format = imgIEncoder::INPUT_FORMAT_HOSTARGB;
        options.AppendInt(32);
        break;
    case gfxASurface::ImageFormatRGB24:
        format = imgIEncoder::INPUT_FORMAT_RGB;
        options.AppendInt(24);
        break;
    default:
        return NS_ERROR_INVALID_ARG;  
    }

    rv = encoder->InitFromData(frame->Data(), 0, frame->Width(),
                               frame->Height(), frame->Stride(),
                               format, options);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t size;
    encoder->GetImageBufferUsed(&size);
    NS_ENSURE_TRUE(size > BFH_LENGTH, NS_ERROR_FAILURE);
    HGLOBAL glob = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT,
                                 size - BFH_LENGTH);
    if (!glob)
        return NS_ERROR_OUT_OF_MEMORY;

    char *dst = (char*) ::GlobalLock(glob);
    char *src;
    rv = encoder->GetImageBuffer(&src);
    NS_ENSURE_SUCCESS(rv, rv);

    ::CopyMemory(dst, src + BFH_LENGTH, size - BFH_LENGTH);
    ::GlobalUnlock(glob);
    
    *outBitmap = (HANDLE)glob;
    return NS_OK;
}

nsImageFromClipboard :: nsImageFromClipboard ()
{
  
}

nsImageFromClipboard :: ~nsImageFromClipboard ( )
{
}






nsresult 
nsImageFromClipboard ::GetEncodedImageStream (unsigned char * aClipboardData, const char * aMIMEFormat, nsIInputStream** aInputStream )
{
  NS_ENSURE_ARG_POINTER (aInputStream);
  NS_ENSURE_ARG_POINTER (aMIMEFormat);
  nsresult rv;
  *aInputStream = nullptr;

  
  
  BITMAPINFO* header = (BITMAPINFO *) aClipboardData;
  int32_t width  = header->bmiHeader.biWidth;
  int32_t height = header->bmiHeader.biHeight;
  
  NS_ENSURE_TRUE(height > 0, NS_ERROR_FAILURE); 

  unsigned char * rgbData = new unsigned char[width * height * 3 ];

  if (rgbData) {
    BYTE  * pGlobal = (BYTE *) aClipboardData;
    
    rv = ConvertColorBitMap((unsigned char *) (pGlobal + header->bmiHeader.biSize), header, rgbData);
    
    if (NS_SUCCEEDED(rv)) {
      nsAutoCString encoderCID(NS_LITERAL_CSTRING("@mozilla.org/image/encoder;2?type="));

      
      if (strcmp(aMIMEFormat, kJPGImageMime) == 0)
        encoderCID.Append("image/jpeg");
      else
        encoderCID.Append(aMIMEFormat);
      nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(encoderCID.get(), &rv);
      if (NS_SUCCEEDED(rv)){
        rv = encoder->InitFromData(rgbData, 0, width, height, 3 * width , 
                                   imgIEncoder::INPUT_FORMAT_RGB, EmptyString());
        if (NS_SUCCEEDED(rv))
          encoder->QueryInterface(NS_GET_IID(nsIInputStream), (void **) aInputStream);
      }
    }
    delete [] rgbData;
  } 
  else 
    rv = NS_ERROR_OUT_OF_MEMORY;

  return rv;
} 






void
nsImageFromClipboard::InvertRows(unsigned char * aInitialBuffer, uint32_t aSizeOfBuffer, uint32_t aNumBytesPerRow)
{
  if (!aNumBytesPerRow) 
    return; 

  uint32_t numRows = aSizeOfBuffer / aNumBytesPerRow;
  unsigned char * row = new unsigned char[aNumBytesPerRow];

  uint32_t currentRow = 0;
  uint32_t lastRow = (numRows - 1) * aNumBytesPerRow;
  while (currentRow < lastRow)
  {
    
    memcpy(row, &aInitialBuffer[currentRow], aNumBytesPerRow);
    memcpy(&aInitialBuffer[currentRow], &aInitialBuffer[lastRow], aNumBytesPerRow);
    memcpy(&aInitialBuffer[lastRow], row, aNumBytesPerRow);
    lastRow -= aNumBytesPerRow;
    currentRow += aNumBytesPerRow;
  }

  delete[] row;
}






nsresult 
nsImageFromClipboard::ConvertColorBitMap(unsigned char * aInputBuffer, PBITMAPINFO pBitMapInfo, unsigned char * aOutBuffer)
{
  uint8_t bitCount = pBitMapInfo->bmiHeader.biBitCount; 
  uint32_t imageSize = pBitMapInfo->bmiHeader.biSizeImage; 
  uint32_t bytesPerPixel = bitCount / 8;
  
  if (bitCount <= 4)
    bytesPerPixel = 1;

  
  
  uint32_t rowSize = (bitCount * pBitMapInfo->bmiHeader.biWidth + 7) / 8; 
  if (rowSize % 4)
    rowSize += (4 - (rowSize % 4)); 
  
  
  if (bitCount <= 8)
  {
    int32_t bytesToSkip = (pBitMapInfo->bmiHeader.biClrUsed ? pBitMapInfo->bmiHeader.biClrUsed : (1 << bitCount) ) * sizeof(RGBQUAD);
    aInputBuffer +=  bytesToSkip;
  }

  bitFields colorMasks; 

  if (pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS)
  {
    
    colorMasks.red = (*((uint32_t*)&(pBitMapInfo->bmiColors[0]))); 
    colorMasks.green = (*((uint32_t*)&(pBitMapInfo->bmiColors[1]))); 
    colorMasks.blue = (*((uint32_t*)&(pBitMapInfo->bmiColors[2]))); 
    CalcBitShift(&colorMasks);
    aInputBuffer += 3 * sizeof(DWORD);
  } 
  else if (pBitMapInfo->bmiHeader.biCompression == BI_RGB && !imageSize)  
  {
    
    imageSize = rowSize * pBitMapInfo->bmiHeader.biHeight;
  }

  
  InvertRows(aInputBuffer, imageSize, rowSize);

  if (!pBitMapInfo->bmiHeader.biCompression || pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS) 
  {  
    uint32_t index = 0;
    uint32_t writeIndex = 0;
     
    unsigned char redValue, greenValue, blueValue;
    uint8_t colorTableEntry = 0;
    int8_t bit; 
    uint32_t numPixelsLeftInRow = pBitMapInfo->bmiHeader.biWidth; 
    uint32_t pos = 0;

    while (index < imageSize)
    {
      switch (bitCount) 
      {
        case 1:
          for (bit = 7; bit >= 0 && numPixelsLeftInRow; bit--)
          {
            colorTableEntry = (aInputBuffer[index] >> bit) & 1;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
            numPixelsLeftInRow--;
          }
          pos += 1;
          break;
        case 4:
          {
            
            
            colorTableEntry = aInputBuffer[index] >> 4;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
            aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
            numPixelsLeftInRow--;

            if (numPixelsLeftInRow) 
            {
              colorTableEntry = aInputBuffer[index] & 0xF;
              aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbRed;
              aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbGreen;
              aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[colorTableEntry].rgbBlue;
              numPixelsLeftInRow--;
            }
            pos += 1;
          }
          break;
        case 8:
          aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[aInputBuffer[index]].rgbRed;
          aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[aInputBuffer[index]].rgbGreen;
          aOutBuffer[writeIndex++] = pBitMapInfo->bmiColors[aInputBuffer[index]].rgbBlue;
          numPixelsLeftInRow--;
          pos += 1;    
          break;
        case 16:
          {
            uint16_t num = 0;
            num = (uint8_t) aInputBuffer[index+1];
            num <<= 8;
            num |= (uint8_t) aInputBuffer[index];

            redValue = ((uint32_t) (((float)(num & 0xf800) / 0xf800) * 0xFF0000) & 0xFF0000)>> 16;
            greenValue =  ((uint32_t)(((float)(num & 0x07E0) / 0x07E0) * 0x00FF00) & 0x00FF00)>> 8;
            blueValue =  ((uint32_t)(((float)(num & 0x001F) / 0x001F) * 0x0000FF) & 0x0000FF);

            
            aOutBuffer[writeIndex++] = redValue;
            aOutBuffer[writeIndex++] = greenValue;
            aOutBuffer[writeIndex++] = blueValue;
            numPixelsLeftInRow--;
            pos += 2;          
          }
          break;
        case 32:
        case 24:
          if (pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS)
          {
            uint32_t val = *((uint32_t*) (aInputBuffer + index) );
            aOutBuffer[writeIndex++] = (val & colorMasks.red) >> colorMasks.redRightShift << colorMasks.redLeftShift;
            aOutBuffer[writeIndex++] =  (val & colorMasks.green) >> colorMasks.greenRightShift << colorMasks.greenLeftShift;
            aOutBuffer[writeIndex++] = (val & colorMasks.blue) >> colorMasks.blueRightShift << colorMasks.blueLeftShift;
            numPixelsLeftInRow--;
            pos += 4; 
          }
          else
          {
            aOutBuffer[writeIndex++] = aInputBuffer[index+2];
            aOutBuffer[writeIndex++] =  aInputBuffer[index+1];
            aOutBuffer[writeIndex++] = aInputBuffer[index];
            numPixelsLeftInRow--;
            pos += bytesPerPixel; 
          }
          break;
        default:
          
          return NS_ERROR_FAILURE;
      }

      index += bytesPerPixel; 

      if (!numPixelsLeftInRow)
      {
        if (rowSize != pos)
        {
          
          index += (rowSize - pos);
        }
        numPixelsLeftInRow = pBitMapInfo->bmiHeader.biWidth;
        pos = 0; 
      }

    } 
  }

  return NS_OK;
}

void nsImageFromClipboard::CalcBitmask(uint32_t aMask, uint8_t& aBegin, uint8_t& aLength)
{
  
  uint8_t pos;
  bool started = false;
  aBegin = aLength = 0;
  for (pos = 0; pos <= 31; pos++) 
  {
    if (!started && (aMask & (1 << pos))) 
    {
      aBegin = pos;
      started = true;
    }
    else if (started && !(aMask & (1 << pos))) 
    {
      aLength = pos - aBegin;
      break;
    }
  }
}

void nsImageFromClipboard::CalcBitShift(bitFields * aColorMask)
{
  uint8_t begin, length;
  
  CalcBitmask(aColorMask->red, begin, length);
  aColorMask->redRightShift = begin;
  aColorMask->redLeftShift = 8 - length;
  
  CalcBitmask(aColorMask->green, begin, length);
  aColorMask->greenRightShift = begin;
  aColorMask->greenLeftShift = 8 - length;
  
  CalcBitmask(aColorMask->blue, begin, length);
  aColorMask->blueRightShift = begin;
  aColorMask->blueLeftShift = 8 - length;
}
