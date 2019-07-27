





#ifndef mozilla_osfileconstants_h__
#define mozilla_osfileconstants_h__

#include "nsIOSFileConstantsService.h"
#include "mozilla/Attributes.h"

namespace mozilla {










nsresult InitOSFileConstants();










void CleanupOSFileConstants();







bool DefineOSFileConstants(JSContext *cx, JS::Handle<JSObject*> global);




class OSFileConstantsService final : public nsIOSFileConstantsService
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOSFILECONSTANTSSERVICE
  OSFileConstantsService();
private:
  ~OSFileConstantsService();
};

} 

#endif 
