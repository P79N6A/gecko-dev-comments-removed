







































#ifndef _nsGIFDecoder2_h
#define _nsGIFDecoder2_h

#include "nsCOMPtr.h"
#include "imgIDecoder.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"

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
  
  nsresult ProcessData(unsigned char *data, PRUint32 count);
  static NS_METHOD ReadDataOut(nsIInputStream* in,
                               void* closure,
                               const char* fromRawSegment,
                               PRUint32 toOffset,
                               PRUint32 count,
                               PRUint32 *writeCount);

private:
  


  void      BeginGIF();
  void      EndGIF(PRBool aSuccess);
  nsresult  BeginImageFrame(gfx_depth aDepth);
  void      EndImageFrame();
  nsresult  FlushImageData();
  nsresult  FlushImageData(PRUint32 fromRow, PRUint32 rows);

  nsresult  GifWrite(const PRUint8 * buf, PRUint32 numbytes);
  PRUint32  OutputRow();
  PRBool    DoLzw(const PRUint8 *q);

  inline int ClearCode() const { return 1 << mGIFStruct.datasize; }

  nsCOMPtr<imgIContainer> mImageContainer;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32 mFlags;
  PRInt32 mCurrentRow;
  PRInt32 mLastFlushedRow;

  PRUint8 *mImageData;       
  PRUint32 *mColormap;       
  PRUint32 mColormapSize;
  PRUint32 mOldColor;        

  
  
  PRInt32 mCurrentFrame;

  PRUint8 mCurrentPass;
  PRUint8 mLastFlushedPass;
  PRUint8 mColorMask;        
  PRPackedBool mGIFOpen;
  PRPackedBool mSawTransparency;
  PRPackedBool mError;
  PRPackedBool mEnded;

  gif_struct mGIFStruct;
};

#endif
