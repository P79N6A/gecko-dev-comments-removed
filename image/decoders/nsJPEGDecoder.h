







































#ifndef nsJPEGDecoder_h__
#define nsJPEGDecoder_h__

#include "RasterImage.h"



#undef INT32

#include "Decoder.h"

#include "nsAutoPtr.h"

#include "imgIDecoderObserver.h"
#include "nsIInputStream.h"
#include "nsIPipe.h"
#include "qcms.h"

extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>

namespace mozilla {
namespace imagelib {

typedef struct {
    struct jpeg_error_mgr pub;  
    jmp_buf setjmp_buffer;      
} decoder_error_mgr;

typedef enum {
    JPEG_HEADER,                          
    JPEG_START_DECOMPRESS,
    JPEG_DECOMPRESS_PROGRESSIVE,          
    JPEG_DECOMPRESS_SEQUENTIAL,           
    JPEG_DONE,
    JPEG_SINK_NON_JPEG_TRAILER,          
                                         
    JPEG_ERROR    
} jstate;

class RasterImage;

class nsJPEGDecoder : public Decoder
{
public:
  nsJPEGDecoder(RasterImage &aImage, imgIDecoderObserver* aObserver);
  virtual ~nsJPEGDecoder();

  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual void FinishInternal();

  virtual Telemetry::ID SpeedHistogram();
  void NotifyDone();

protected:
  void OutputScanlines(bool* suspend);

public:
  PRUint8 *mImageData;

  struct jpeg_decompress_struct mInfo;
  struct jpeg_source_mgr mSourceMgr;
  decoder_error_mgr mErr;
  jstate mState;

  PRUint32 mBytesToSkip;

  const JOCTET *mSegment;   
  PRUint32 mSegmentLen;     

  JOCTET *mBackBuffer;
  PRUint32 mBackBufferLen; 
  PRUint32 mBackBufferSize; 
  PRUint32 mBackBufferUnreadLen; 

  JOCTET  *mProfile;
  PRUint32 mProfileLength;

  qcms_profile *mInProfile;
  qcms_transform *mTransform;

  bool mReading;

  PRUint32 mCMSMode;
};

} 
} 

#endif 
