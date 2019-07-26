



#ifndef URL_h___
#define URL_h___

#include "nscore.h"
#include "mozilla/dom/URLBinding.h"

class nsIDOMBlob;

namespace mozilla {
namespace dom {

class URL MOZ_FINAL
{
public:
  
  static void CreateObjectURL(nsISupports* aGlobal, nsIDOMBlob* aBlob,
                              const objectURLOptions& aOptions,
                              nsAString& aResult,
                              ErrorResult& aError);
  static void RevokeObjectURL(nsISupports* aGlobal, const nsAString& aURL);
};

}
}

#endif 
