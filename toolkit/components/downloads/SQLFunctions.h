




#ifndef mozilla_downloads_SQLFunctions_h
#define mozilla_downloads_SQLFunctions_h

#include "mozIStorageFunction.h"
#include "mozilla/Attributes.h"

class nsCString;
class mozIStorageConnection;

namespace mozilla {
namespace downloads {








class GenerateGUIDFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





   static nsresult create(mozIStorageConnection *aDBConn);
};

nsresult GenerateGUID(nsCString& _guid);

} 
} 

#endif
