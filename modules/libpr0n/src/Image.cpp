




































#include "Image.h"

namespace mozilla {
namespace imagelib {


Image::Image(imgStatusTracker* aStatusTracker) :
  mInitialized(PR_FALSE)
{
  if (aStatusTracker) {
    mStatusTracker = aStatusTracker;
    mStatusTracker->SetImage(this);
  } else {
    mStatusTracker = new imgStatusTracker(this);
  }
}


Image::eDecoderType
Image::GetDecoderType(const char *aMimeType)
{
  
  eDecoderType rv = eDecoderType_unknown;

  
  if (!strcmp(aMimeType, "image/png"))
    rv = eDecoderType_png;
  else if (!strcmp(aMimeType, "image/x-png"))
    rv = eDecoderType_png;

  
  else if (!strcmp(aMimeType, "image/gif"))
    rv = eDecoderType_gif;


  
  else if (!strcmp(aMimeType, "image/jpeg"))
    rv = eDecoderType_jpeg;
  else if (!strcmp(aMimeType, "image/pjpeg"))
    rv = eDecoderType_jpeg;
  else if (!strcmp(aMimeType, "image/jpg"))
    rv = eDecoderType_jpeg;

  
  else if (!strcmp(aMimeType, "image/bmp"))
    rv = eDecoderType_bmp;
  else if (!strcmp(aMimeType, "image/x-ms-bmp"))
    rv = eDecoderType_bmp;


  
  else if (!strcmp(aMimeType, "image/x-icon"))
    rv = eDecoderType_ico;
  else if (!strcmp(aMimeType, "image/vnd.microsoft.icon"))
    rv = eDecoderType_ico;

  
  else if (!strcmp(aMimeType, "image/icon"))
    rv = eDecoderType_icon;

  return rv;
}


} 
} 
