







































#ifndef nsJPEGDecoder_h__
#define nsJPEGDecoder_h__

#include "RasterImage.h"



#undef INT32

#include "imgIDecoder.h"

#include "nsAutoPtr.h"

#include "imgIDecoderObserver.h"
#include "nsIInputStream.h"
#include "nsIPipe.h"
#include "qcms.h"

extern "C" {
#include "jpeglib.h"
}

#include <setjmp.h>

#define NS_JPEGDECODER_CID \
{ /* 5871a422-1dd2-11b2-ab3f-e2e56be5da9c */         \
     0x5871a422,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0xab, 0x3f, 0xe2, 0xe5, 0x6b, 0xe5, 0xda, 0x9c} \
}

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

namespace mozilla {
namespace imagelib {
class RasterImage;
} 
} 

class nsJPEGDecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsJPEGDecoder();
  virtual ~nsJPEGDecoder();

  void NotifyDone(PRBool aSuccess);

protected:
  nsresult OutputScanlines(PRBool* suspend);

public:
  nsRefPtr<mozilla::imagelib::RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;

  PRUint32 mFlags;
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

  PRPackedBool mReading;
  PRPackedBool mNotifiedDone;
};

#endif 
