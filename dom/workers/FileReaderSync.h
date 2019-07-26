





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
  Constructor(JSContext* aCx, JSObject* aGlobal, ErrorResult& aRv);

  NS_DECL_ISUPPORTS_INHERITED

  FileReaderSync(JSContext* aCx);

  JSObject* ReadAsArrayBuffer(JSContext* aCx, JSObject* aBlob,
                              ErrorResult& aRv);
  void ReadAsBinaryString(JSObject* aBlob, nsAString& aResult,
                          ErrorResult& aRv);
  void ReadAsText(JSObject* aBlob, const Optional<nsAString>& aEncoding,
                  nsAString& aResult, ErrorResult& aRv);
  void ReadAsDataURL(JSObject* aBlob, nsAString& aResult, ErrorResult& aRv);

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);
};

END_WORKERS_NAMESPACE

#endif 
