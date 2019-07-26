



#ifndef URL_h___
#define URL_h___

#include "nscore.h"
#include "nsString.h"

class nsIDOMBlob;
class nsISupports;

namespace mozilla {

class ErrorResult;
class DOMMediaStream;

namespace dom {

class MediaSource;
class GlobalObject;
struct objectURLOptions;

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
  static void CreateObjectURL(const GlobalObject& aGlobal,
                              MediaSource& aSource,
                              const objectURLOptions& aOptions,
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
