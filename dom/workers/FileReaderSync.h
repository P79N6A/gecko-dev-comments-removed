





#ifndef mozilla_dom_workers_filereadersync_h__
#define mozilla_dom_workers_filereadersync_h__

#include "Workers.h"

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

class FileReaderSync MOZ_FINAL
{
  NS_INLINE_DECL_REFCOUNTING(FileReaderSync)

private:
  
  ~FileReaderSync()
  {
  }

  nsresult ConvertStream(nsIInputStream *aStream, const char *aCharset,
                         nsAString &aResult);

public:
  static already_AddRefed<FileReaderSync>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  JSObject* WrapObject(JSContext* aCx);

  void ReadAsArrayBuffer(JSContext* aCx, JS::Handle<JSObject*> aScopeObj,
                         JS::Handle<JSObject*> aBlob,
                         JS::MutableHandle<JSObject*> aRetval,
                         ErrorResult& aRv);
  void ReadAsBinaryString(JS::Handle<JSObject*> aBlob, nsAString& aResult,
                          ErrorResult& aRv);
  void ReadAsText(JS::Handle<JSObject*> aBlob,
                  const Optional<nsAString>& aEncoding,
                  nsAString& aResult, ErrorResult& aRv);
  void ReadAsDataURL(JS::Handle<JSObject*> aBlob, nsAString& aResult,
                     ErrorResult& aRv);
};

END_WORKERS_NAMESPACE

#endif 
