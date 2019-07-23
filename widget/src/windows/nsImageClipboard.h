






































#ifndef WINCE








#include "nsError.h"
#include <windows.h>

#include "nsCOMPtr.h"
#include "nsIImage.h"
#include "nsIInputStream.h"








class nsImageToClipboard
{
public:
  nsImageToClipboard ( nsIImage* inImage );
  ~nsImageToClipboard();

    
    
    
    
  nsresult GetPicture ( HANDLE* outBits ) ;

private:

    
  PRInt32 CalcSize(PRInt32 aHeight, PRInt32 aColors, WORD aBitsPerPixel, PRInt32 aSpanBytes);
  PRInt32 CalcSpanLength(PRUint32 aWidth, PRUint32 aBitCount);

    
  nsresult CreateFromImage ( nsIImage* inImage, HANDLE* outBitmap );

  nsCOMPtr<nsIImage> mImage;            

}; 


struct bitFields {
    PRUint32 red;
    PRUint32 green;
    PRUint32 blue;
    PRUint8 redLeftShift;
    PRUint8 redRightShift;
    PRUint8 greenLeftShift;
    PRUint8 greenRightShift;
    PRUint8 blueLeftShift;
    PRUint8 blueRightShift;
};







class nsImageFromClipboard
{
public:
  nsImageFromClipboard () ;
  ~nsImageFromClipboard ( ) ;
  
    
  nsresult GetEncodedImageStream (unsigned char * aClipboardData, nsIInputStream** outImage);

private:

  void InvertRows(unsigned char * aInitialBuffer, PRUint32 aSizeOfBuffer, PRUint32 aNumBytesPerRow);
  nsresult ConvertColorBitMap(unsigned char * aInputBuffer, PBITMAPINFO pBitMapInfo, unsigned char * aOutBuffer);
  void CalcBitmask(PRUint32 aMask, PRUint8& aBegin, PRUint8& aLength);
  void CalcBitShift(bitFields * aColorMask);

}; 

#endif 
