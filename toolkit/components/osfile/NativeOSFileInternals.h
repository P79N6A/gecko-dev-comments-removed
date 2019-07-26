



#ifndef mozilla_nativeosfileinternalservice_h__
#define mozilla_nativeosfileinternalservice_h__

#include "nsINativeOSFileInternals.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class NativeOSFileInternalsService MOZ_FINAL : public nsINativeOSFileInternalsService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINATIVEOSFILEINTERNALSSERVICE
private:
  ~NativeOSFileInternalsService() {}
  
  void operator=(const NativeOSFileInternalsService& other) MOZ_DELETE;
};

} 

#endif 
