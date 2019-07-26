





#ifndef mozilla_dom_workers_filereadersync_h__
#define mozilla_dom_workers_filereadersync_h__

#include "Workers.h"
#include "mozilla/dom/workers/bindings/DOMBindingBase.h"

#include "nsICharsetDetectionObserver.h"
#include "nsStringGlue.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"

class nsIInputStream;
class nsIDOMBlob;

BEGIN_WORKERS_NAMESPACE

class FileReaderSync MOZ_FINAL : public DOMBindingBase,
                                 public nsICharsetDetectionObserver
{
  nsCString mCharset;
  nsresult ConvertStream(nsIInputStream *aStream, const char *aCharset,
                         nsAString &aResult);
  nsresult GuessCharset(nsIInputStream *aStream, nsACString &aCharset);

public:
  virtual void
  _trace(JSTracer* aTrc) MOZ_OVERRIDE;

  virtual void
  _finalize(JSFreeOp* aFop) MOZ_OVERRIDE;

  static FileReaderSync*
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  NS_DECL_ISUPPORTS_INHERITED

  FileReaderSync(JSContext* aCx);

  JSObject* ReadAsArrayBuffer(JSContext* aCx, JS::Handle<JSObject*> aBlob,
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
