



#ifndef URL_h___
#define URL_h___

#include "nscore.h"
#include "mozilla/dom/URLBinding.h"

class nsIDOMBlob;
class nsIDOMMediaStream;

namespace mozilla {
namespace dom {

class URL MOZ_FINAL
{
public:
  
  static void CreateObjectURL(nsISupports* aGlobal, nsIDOMBlob* aBlob,
                              const objectURLOptions& aOptions,
                              nsAString& aResult,
                              ErrorResult& aError);
  static void CreateObjectURL(nsISupports* aGlobal, nsIDOMMediaStream* aStream,
                              const mozilla::dom::objectURLOptions& aOptions,
                              nsAString& aResult,
                              mozilla::ErrorResult& aError);
  static void RevokeObjectURL(nsISupports* aGlobal, const nsAString& aURL);

private:
  static void CreateObjectURLInternal(nsISupports* aGlobal, nsISupports* aObject,
                                      const nsACString& aScheme,
                                      const mozilla::dom::objectURLOptions& aOptions,
                                      nsAString& aResult,
                                      mozilla::ErrorResult& aError);
};

}
}

#endif 
