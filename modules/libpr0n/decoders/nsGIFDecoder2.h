







































#ifndef _nsGIFDecoder2_h
#define _nsGIFDecoder2_h

#include "nsCOMPtr.h"
#include "Decoder.h"
#include "imgIDecoderObserver.h"

#include "GIF2.h"

namespace mozilla {
namespace imagelib {
class RasterImage;




class nsGIFDecoder2 : public Decoder
{
public:

  nsGIFDecoder2();
  ~nsGIFDecoder2();

  virtual nsresult InitInternal();
  virtual nsresult WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual nsresult FinishInternal();

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

} 
} 

#endif
