






































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

private:
  


  void      BeginGIF();
  void      EndGIF();
  void      BeginImageFrame();
  void      EndImageFrame();
  void      FlushImageData();

  nsresult  GifWrite(const PRUint8 * buf, PRUint32 numbytes);
  PRUint32  OutputRow();
  PRBool    DoLzw(const PRUint8 *q);
  void      HaveDecodedRow(
    PRUint8* aRowBufPtr,   
    int aRow,              
    int aDuplicateCount,   
    int aInterlacePass);

  inline int ClearCode() const { return 1 << mGIFStruct.datasize; }

  nsCOMPtr<imgIContainer> mImageContainer;
  nsCOMPtr<gfxIImageFrame> mImageFrame;
  nsCOMPtr<imgIDecoderObserver> mObserver; 
  PRInt32 mCurrentRow;
  PRInt32 mLastFlushedRow;

  PRUint8 *mRGBLine;
  PRUint32 mRGBLineMaxSize;
  PRUint8 mCurrentPass;
  PRUint8 mLastFlushedPass;
  PRPackedBool mGIFOpen;

  gif_struct mGIFStruct;
};

#endif
