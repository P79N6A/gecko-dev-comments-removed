





#ifndef mozilla_dom_workers_filereadersync_h__
#define mozilla_dom_workers_filereadersync_h__

#include "Workers.h"

class nsIInputStream;

namespace mozilla {
class ErrorResult;

namespace dom {
class File;
class GlobalObject;
template<typename> class Optional;
}
}

BEGIN_WORKERS_NAMESPACE

class FileReaderSync final
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

  bool WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector);

  void ReadAsArrayBuffer(JSContext* aCx, JS::Handle<JSObject*> aScopeObj,
                         File& aBlob, JS::MutableHandle<JSObject*> aRetval,
                         ErrorResult& aRv);
  void ReadAsBinaryString(File& aBlob, nsAString& aResult, ErrorResult& aRv);
  void ReadAsText(File& aBlob, const Optional<nsAString>& aEncoding,
                  nsAString& aResult, ErrorResult& aRv);
  void ReadAsDataURL(File& aBlob, nsAString& aResult, ErrorResult& aRv);
};

END_WORKERS_NAMESPACE

#endif 
