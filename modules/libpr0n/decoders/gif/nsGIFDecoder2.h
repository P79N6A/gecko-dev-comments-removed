






































#ifndef _nsGIFDecoder2_h
#define _nsGIFDecoder2_h

#include "nsCOMPtr.h"
#include "imgIDecoder.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxIImageFrame.h"

#include "GIF2.h"

#define NS_GIFDECODER2_CID \
{ /* 797bec5a-1dd2-11b2-a7f8-ca397e0179c4 */         \
     0x797bec5a,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0xa7, 0xf8, 0xca, 0x39, 0x7e, 0x01, 0x79, 0xc4} \
}




class nsGIFDecoder2 : public imgIDecoder   
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsGIFDecoder2();
  ~nsGIFDecoder2();
  
  nsresult ProcessData(unsigned char *data, PRUint32 count, PRUint32 *_retval);

  NS_METHOD FlushImageData();

  


  static int BeginGIF(
    void* aClientData,
    PRUint32 aLogicalScreenWidth, 
    PRUint32 aLogicalScreenHeight,
    PRUint8  aBackgroundRGBIndex);
    
  static int EndGIF(
    void*    aClientData,
    int      aAnimationLoopCount);
  
  static int BeginImageFrame(
    void*    aClientData,
    PRUint32 aFrameNumber,   
    PRUint32 aFrameXOffset,  
    PRUint32 aFrameYOffset,  
    PRUint32 aFrameWidth,    
    PRUint32 aFrameHeight);
  
  static int EndImageFrame(
    void* aClientData,
    PRUint32 aFrameNumber,
    PRUint32 aDelayTimeout);
  
  static int HaveDecodedRow(
    void* aClientData,
    PRUint8* aRowBufPtr,   
    int aRow,              
    int aDuplicateCount,   
    int aInterlacePass);

private:
  nsCOMPtr<imgIContainer> mImageContainer;
  nsCOMPtr<gfxIImageFrame> mImageFrame;
  nsCOMPtr<imgIDecoderObserver> mObserver; 
  PRInt32 mCurrentRow;
  PRInt32 mLastFlushedRow;

  gif_struct *mGIFStruct;

  PRUint8 *mRGBLine;
  PRUint32 mRGBLineMaxSize;
  PRUint8 mBackgroundRGBIndex;
  PRUint8 mCurrentPass;
  PRUint8 mLastFlushedPass;
  PRPackedBool mGIFOpen;
};

void nsGifShutdown();

#endif
