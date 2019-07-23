





































#include "imgIEncoder.h"


#include <stdio.h>

extern "C" {
#include "jpeglib.h"
}

#define NS_JPEGENCODER_CID \
{ /* ac2bb8fe-eeeb-4572-b40f-be03932b56e0 */         \
     0xac2bb8fe,                                     \
     0xeeeb,                                         \
     0x4572,                                         \
    {0xb4, 0x0f, 0xbe, 0x03, 0x93, 0x2b, 0x56, 0xe0} \
}




class nsJPEGEncoder : public imgIEncoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM

  nsJPEGEncoder();

private:
  ~nsJPEGEncoder();

protected:

  void ConvertHostARGBRow(const PRUint8* aSrc, PRUint8* aDest,
                          PRUint32 aPixelWidth);
  void StripAlpha(const PRUint8* aSrc, PRUint8* aDest, PRUint32 aPixelWidth);

  static void initDestination(jpeg_compress_struct* cinfo);
  static boolean emptyOutputBuffer(jpeg_compress_struct* cinfo);
  static void termDestination(jpeg_compress_struct* cinfo);

  static void errorExit(jpeg_common_struct* cinfo);

  
  PRUint8* mImageBuffer;
  PRUint32 mImageBufferSize;
  PRUint32 mImageBufferUsed;

  PRUint32 mImageBufferReadPoint;
};
