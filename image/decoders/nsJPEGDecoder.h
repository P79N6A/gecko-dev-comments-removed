





#ifndef nsJPEGDecoder_h__
#define nsJPEGDecoder_h__

#include "RasterImage.h"



#undef INT32

#include "Decoder.h"

#include "nsAutoPtr.h"

#include "nsIInputStream.h"
#include "nsIPipe.h"
#include "qcms.h"

extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>

namespace mozilla {
namespace image {

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
  nsJPEGDecoder(RasterImage &aImage, Decoder::DecodeStyle aDecodeStyle);
  virtual ~nsJPEGDecoder();

  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);
  virtual void FinishInternal();

  virtual Telemetry::ID SpeedHistogram();
  void NotifyDone();

protected:
  void OutputScanlines(bool* suspend);

public:
  struct jpeg_decompress_struct mInfo;
  struct jpeg_source_mgr mSourceMgr;
  decoder_error_mgr mErr;
  jstate mState;

  uint32_t mBytesToSkip;

  const JOCTET *mSegment;   
  uint32_t mSegmentLen;     

  JOCTET *mBackBuffer;
  uint32_t mBackBufferLen; 
  uint32_t mBackBufferSize; 
  uint32_t mBackBufferUnreadLen; 

  JOCTET  *mProfile;
  uint32_t mProfileLength;

  qcms_profile *mInProfile;
  qcms_transform *mTransform;

  bool mReading;

  const Decoder::DecodeStyle mDecodeStyle;

  uint32_t mCMSMode;
};

} 
} 

#endif 
