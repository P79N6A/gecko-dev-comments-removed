





#ifndef mozilla_dom_workers_filereadersync_h__
#define mozilla_dom_workers_filereadersync_h__

#include "Workers.h"

#include "nsICharsetDetectionObserver.h"

class nsIInputStream;
class nsIDOMBlob;

namespace mozilla {
class ErrorResult;

namespace dom {
class GlobalObject;
template<typename> class Optional;
}
}

BEGIN_WORKERS_NAMESPACE

class FileReaderSync MOZ_FINAL : public nsICharsetDetectionObserver
{
  nsCString mCharset;
  nsresult ConvertStream(nsIInputStream *aStream, const char *aCharset,
                         nsAString &aResult);
  nsresult GuessCharset(nsIInputStream *aStream, nsACString &aCharset);

public:
  static already_AddRefed<FileReaderSync>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  JSObject* WrapObject(JSContext* aCx, JS::HandleObject aScope);

  NS_DECL_ISUPPORTS

  JSObject* ReadAsArrayBuffer(JSContext* aCx, JS::Handle<JSObject*> aScopeObj,
                              JS::Handle<JSObject*> aBlob,
                              ErrorResult& aRv);
  void ReadAsBinaryString(JS::Handle<JSObject*> aBlob, nsAString& aResult,
                          ErrorResult& aRv);
  void ReadAsText(JS::Handle<JSObject*> aBlob,
                  const Optional<nsAString>& aEncoding,
                  nsAString& aResult, ErrorResult& aRv);
  void ReadAsDataURL(JS::Handle<JSObject*> aBlob, nsAString& aResult,
                     ErrorResult& aRv);

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf) MOZ_OVERRIDE;
};

END_WORKERS_NAMESPACE

#endif 
