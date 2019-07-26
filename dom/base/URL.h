



#ifndef URL_h___
#define URL_h___

#include "nscore.h"
#include "mozilla/dom/URLBinding.h"

class nsIDOMBlob;

namespace mozilla {

class DOMMediaStream;

namespace dom {

class URL MOZ_FINAL
{
public:
  
  static void CreateObjectURL(const GlobalObject& aGlobal, nsIDOMBlob* aBlob,
                              const objectURLOptions& aOptions,
                              nsString& aResult,
                              ErrorResult& aError);
  static void CreateObjectURL(const GlobalObject& aGlobal,
                              DOMMediaStream& aStream,
                              const mozilla::dom::objectURLOptions& aOptions,
                              nsString& aResult,
                              mozilla::ErrorResult& aError);
  static void RevokeObjectURL(const GlobalObject& aGlobal,
                              const nsAString& aURL);

private:
  static void CreateObjectURLInternal(nsISupports* aGlobal, nsISupports* aObject,
                                      const nsACString& aScheme,
                                      const mozilla::dom::objectURLOptions& aOptions,
                                      nsString& aResult,
                                      mozilla::ErrorResult& aError);
};

}
}

#endif 
