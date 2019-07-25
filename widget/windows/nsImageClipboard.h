











#include "nsError.h"
#include <windows.h>

#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIInputStream.h"








class nsImageToClipboard
{
public:
  nsImageToClipboard ( imgIContainer* inImage );
  ~nsImageToClipboard();

    
    
    
    
  nsresult GetPicture ( HANDLE* outBits ) ;

private:

    
  int32_t CalcSize(int32_t aHeight, int32_t aColors, WORD aBitsPerPixel, int32_t aSpanBytes);
  int32_t CalcSpanLength(uint32_t aWidth, uint32_t aBitCount);

    
  nsresult CreateFromImage ( imgIContainer* inImage, HANDLE* outBitmap );

  nsCOMPtr<imgIContainer> mImage;            

}; 


struct bitFields {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint8_t redLeftShift;
    uint8_t redRightShift;
    uint8_t greenLeftShift;
    uint8_t greenRightShift;
    uint8_t blueLeftShift;
    uint8_t blueRightShift;
};







class nsImageFromClipboard
{
public:
  nsImageFromClipboard () ;
  ~nsImageFromClipboard ( ) ;
  
    
  nsresult GetEncodedImageStream (unsigned char * aClipboardData, const char * aMIMEFormat, nsIInputStream** outImage);

private:

  void InvertRows(unsigned char * aInitialBuffer, uint32_t aSizeOfBuffer, uint32_t aNumBytesPerRow);
  nsresult ConvertColorBitMap(unsigned char * aInputBuffer, PBITMAPINFO pBitMapInfo, unsigned char * aOutBuffer);
  void CalcBitmask(uint32_t aMask, uint8_t& aBegin, uint8_t& aLength);
  void CalcBitShift(bitFields * aColorMask);

}; 
