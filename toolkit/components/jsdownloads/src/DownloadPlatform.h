



#ifndef __DownloadPlatform_h__
#define __DownloadPlatform_h__

#include "mozIDownloadPlatform.h"

#include "nsCOMPtr.h"

class nsIURI;
class nsIFile;

class DownloadPlatform : public mozIDownloadPlatform
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_MOZIDOWNLOADPLATFORM

  DownloadPlatform() { }
  virtual ~DownloadPlatform() { }

  static DownloadPlatform *gDownloadPlatformService;

  static DownloadPlatform* GetDownloadPlatform();
};

#endif
