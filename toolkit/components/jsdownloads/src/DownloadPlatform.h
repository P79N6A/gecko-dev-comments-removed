



#ifndef __DownloadPlatform_h__
#define __DownloadPlatform_h__

#include "mozIDownloadPlatform.h"

#include "nsCOMPtr.h"

class DownloadPlatform : public mozIDownloadPlatform
{
protected:

  virtual ~DownloadPlatform() { }

public:

  NS_DECL_ISUPPORTS
  NS_DECL_MOZIDOWNLOADPLATFORM

  DownloadPlatform() { }

  static DownloadPlatform *gDownloadPlatformService;

  static DownloadPlatform* GetDownloadPlatform();
};

#endif
