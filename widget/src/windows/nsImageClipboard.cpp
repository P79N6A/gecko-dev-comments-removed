







































 
 
#include "nsITransferable.h"
#include "nsImageClipboard.h"
#include "nsGfxCIID.h"
#include "nsMemory.h"
#include "prmem.h"
#include "imgIEncoder.h"
#include "nsLiteralString.h"














nsImageToClipboard :: nsImageToClipboard ( imgIContainer* inImage )
  : mImage(inImage)
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







PRInt32 
nsImageToClipboard :: CalcSize ( PRInt32 aHeight, PRInt32 aColors, WORD aBitsPerPixel, PRInt32 aSpanBytes )
{
  PRInt32 HeaderMem = sizeof(BITMAPINFOHEADER);

  
  if (aBitsPerPixel < 16)
    HeaderMem += aColors * sizeof(RGBQUAD);

  if (aHeight < 0)
    aHeight = -aHeight;

  return (HeaderMem + (aHeight * aSpanBytes));
}







PRInt32 
nsImageToClipboard::CalcSpanLength(PRUint32 aWidth, PRUint32 aBitCount)
{
  PRInt32 spanBytes = (aWidth * aBitCount) >> 5;
  
  if ((aWidth * aBitCount) & 0x1F)
    spanBytes++;
  spanBytes <<= 2;

  return spanBytes;
}








nsresult
nsImageToClipboard::CreateFromImage ( imgIContainer* inImage, HANDLE* outBitmap )
{
    nsresult result = NS_OK;
    *outBitmap = nsnull;

    nsRefPtr<gfxImageSurface> frame;
    nsresult rv = inImage->CopyFrame(imgIContainer::FRAME_CURRENT,
                                     imgIContainer::FLAG_SYNC_DECODE,
                                     getter_AddRefs(frame));
    if (NS_FAILED(rv))
      return rv;

    const PRUint32 imageSize = frame->GetDataSize();
    const PRInt32 bitmapSize = sizeof(BITMAPINFOHEADER) + imageSize;

    HGLOBAL glob = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT, bitmapSize);
    if (!glob) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    void *data = (void*)::GlobalLock(glob);

    BITMAPINFOHEADER *header = (BITMAPINFOHEADER*)data;
    header->biSize          = sizeof(BITMAPINFOHEADER);
    header->biWidth         = frame->Width();
    header->biHeight        = frame->Height();

    header->biPlanes        = 1;
    if (frame->Format() == gfxASurface::ImageFormatARGB32)
        header->biBitCount  = 32;
    else if (frame->Format() == gfxASurface::ImageFormatRGB24)
        header->biBitCount  = 24;
    header->biCompression   = BI_RGB;
    header->biSizeImage     = imageSize;

    const PRUint32 bpr = frame->Stride();

    BYTE *dstBits = (BYTE*)data + sizeof(BITMAPINFOHEADER);
    BYTE *srcBits = frame->Data();
    for (PRInt32 i = 0; i < header->biHeight; ++i) {
        PRUint32 srcOffset = imageSize - (bpr * (i + 1));
        PRUint32 dstOffset = i * bpr;
        ::CopyMemory(dstBits + dstOffset, srcBits + srcOffset, bpr);
    }
    
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
  *aInputStream = nsnull;

  
  
  BITMAPINFO* header = (BITMAPINFO *) aClipboardData;
  PRInt32 width  = header->bmiHeader.biWidth;
  PRInt32 height = header->bmiHeader.biHeight;
  
  NS_ENSURE_TRUE(height > 0, NS_ERROR_FAILURE); 

  unsigned char * rgbData = new unsigned char[width * height * 3 ];

  if (rgbData) {
    BYTE  * pGlobal = (BYTE *) aClipboardData;
    
    rv = ConvertColorBitMap((unsigned char *) (pGlobal + header->bmiHeader.biSize), header, rgbData);
    
    if (NS_SUCCEEDED(rv)) {
      nsCAutoString encoderCID(NS_LITERAL_CSTRING("@mozilla.org/image/encoder;2?type="));

      
      if (strcmp(aMIMEFormat, kJPEGImageMime) == 0)
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
nsImageFromClipboard::InvertRows(unsigned char * aInitialBuffer, PRUint32 aSizeOfBuffer, PRUint32 aNumBytesPerRow)
{
  if (!aNumBytesPerRow) 
    return; 

  PRUint32 numRows = aSizeOfBuffer / aNumBytesPerRow;
  unsigned char * row = new unsigned char[aNumBytesPerRow];

  PRUint32 currentRow = 0;
  PRUint32 lastRow = (numRows - 1) * aNumBytesPerRow;
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
  PRUint8 bitCount = pBitMapInfo->bmiHeader.biBitCount; 
  PRUint32 imageSize = pBitMapInfo->bmiHeader.biSizeImage; 
  PRUint32 bytesPerPixel = bitCount / 8;
  
  if (bitCount <= 4)
    bytesPerPixel = 1;

  
  
  PRUint32 rowSize = (bitCount * pBitMapInfo->bmiHeader.biWidth + 7) / 8; 
  if (rowSize % 4)
    rowSize += (4 - (rowSize % 4)); 
  
  
  if (bitCount <= 8)
  {
    PRInt32 bytesToSkip = (pBitMapInfo->bmiHeader.biClrUsed ? pBitMapInfo->bmiHeader.biClrUsed : (1 << bitCount) ) * sizeof(RGBQUAD);
    aInputBuffer +=  bytesToSkip;
  }

  bitFields colorMasks; 

  if (pBitMapInfo->bmiHeader.biCompression == BI_BITFIELDS)
  {
    
    colorMasks.red = (*((PRUint32*)&(pBitMapInfo->bmiColors[0]))); 
    colorMasks.green = (*((PRUint32*)&(pBitMapInfo->bmiColors[1]))); 
    colorMasks.blue = (*((PRUint32*)&(pBitMapInfo->bmiColors[2]))); 
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
    PRUint32 index = 0;
    PRUint32 writeIndex = 0;
     
    unsigned char redValue, greenValue, blueValue;
    PRUint8 colorTableEntry = 0;
    PRInt8 bit; 
    PRUint32 numPixelsLeftInRow = pBitMapInfo->bmiHeader.biWidth; 
    PRUint32 pos = 0;

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
            PRUint16 num = 0;
            num = (PRUint8) aInputBuffer[index+1];
            num <<= 8;
            num |= (PRUint8) aInputBuffer[index];

            redValue = ((PRUint32) (((float)(num & 0xf800) / 0xf800) * 0xFF0000) & 0xFF0000)>> 16;
            greenValue =  ((PRUint32)(((float)(num & 0x07E0) / 0x07E0) * 0x00FF00) & 0x00FF00)>> 8;
            blueValue =  ((PRUint32)(((float)(num & 0x001F) / 0x001F) * 0x0000FF) & 0x0000FF);

            
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
            PRUint32 val = *((PRUint32*) (aInputBuffer + index) );
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

void nsImageFromClipboard::CalcBitmask(PRUint32 aMask, PRUint8& aBegin, PRUint8& aLength)
{
  
  PRUint8 pos;
  PRBool started = PR_FALSE;
  aBegin = aLength = 0;
  for (pos = 0; pos <= 31; pos++) 
  {
    if (!started && (aMask & (1 << pos))) 
    {
      aBegin = pos;
      started = PR_TRUE;
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
  PRUint8 begin, length;
  
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
