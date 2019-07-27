




#ifndef nsDOMFileReader_h__
#define nsDOMFileReader_h__

#include "mozilla/Attributes.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsJSUtils.h"
#include "nsTArray.h"
#include "nsIJSNativeInitializer.h"
#include "prtime.h"
#include "nsITimer.h"
#include "nsIAsyncInputStream.h"

#include "nsIDOMFileReader.h"
#include "nsIDOMFileList.h"
#include "nsCOMPtr.h"

#include "FileIOObject.h"

namespace mozilla {
namespace dom {
class DOMFile;
}
}

class nsDOMFileReader : public mozilla::dom::FileIOObject,
                        public nsIDOMFileReader,
                        public nsIInterfaceRequestor,
                        public nsSupportsWeakReference
{
  typedef mozilla::ErrorResult ErrorResult;
  typedef mozilla::dom::GlobalObject GlobalObject;
  typedef mozilla::dom::DOMFile DOMFile;
public:
  nsDOMFileReader();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMFILEREADER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(mozilla::DOMEventTargetHelper)

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  virtual void DoAbort(nsAString& aEvent) MOZ_OVERRIDE;

  virtual nsresult DoReadData(nsIAsyncInputStream* aStream, uint64_t aCount) MOZ_OVERRIDE;

  virtual nsresult DoOnLoadEnd(nsresult aStatus, nsAString& aSuccessEvent,
                               nsAString& aTerminationEvent) MOZ_OVERRIDE;

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  static already_AddRefed<nsDOMFileReader>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);
  void ReadAsArrayBuffer(JSContext* aCx, DOMFile& aBlob, ErrorResult& aRv)
  {
    ReadFileContent(aCx, aBlob, EmptyString(), FILE_AS_ARRAYBUFFER, aRv);
  }

  void ReadAsText(DOMFile& aBlob, const nsAString& aLabel, ErrorResult& aRv)
  {
    ReadFileContent(nullptr, aBlob, aLabel, FILE_AS_TEXT, aRv);
  }

  void ReadAsDataURL(DOMFile& aBlob, ErrorResult& aRv)
  {
    ReadFileContent(nullptr, aBlob, EmptyString(), FILE_AS_DATAURL, aRv);
  }

  using FileIOObject::Abort;

  

  void GetResult(JSContext* aCx, JS::MutableHandle<JS::Value> aResult,
                 ErrorResult& aRv);

  using FileIOObject::GetError;

  IMPL_EVENT_HANDLER(loadstart)
  using FileIOObject::GetOnprogress;
  using FileIOObject::SetOnprogress;
  IMPL_EVENT_HANDLER(load)
  using FileIOObject::GetOnabort;
  using FileIOObject::SetOnabort;
  using FileIOObject::GetOnerror;
  using FileIOObject::SetOnerror;
  IMPL_EVENT_HANDLER(loadend)

  void ReadAsBinaryString(DOMFile& aBlob, ErrorResult& aRv)
  {
    ReadFileContent(nullptr, aBlob, EmptyString(), FILE_AS_BINARY, aRv);
  }


  nsresult Init();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMFileReader,
                                                         FileIOObject)
  void RootResultArrayBuffer();

protected:
  virtual ~nsDOMFileReader();

  enum eDataFormat {
    FILE_AS_ARRAYBUFFER,
    FILE_AS_BINARY,
    FILE_AS_TEXT,
    FILE_AS_DATAURL
  };

  void ReadFileContent(JSContext* aCx, DOMFile& aBlob,
                       const nsAString &aCharset, eDataFormat aDataFormat,
                       ErrorResult& aRv);
  nsresult GetAsText(nsIDOMBlob *aFile, const nsACString &aCharset,
                     const char *aFileData, uint32_t aDataLen, nsAString &aResult);
  nsresult GetAsDataURL(nsIDOMBlob *aFile, const char *aFileData, uint32_t aDataLen, nsAString &aResult);

  void FreeFileData() {
    moz_free(mFileData);
    mFileData = nullptr;
    mDataLen = 0;
  }

  char *mFileData;
  nsCOMPtr<nsIDOMBlob> mFile;
  nsCString mCharset;
  uint32_t mDataLen;

  eDataFormat mDataFormat;

  nsString mResult;

  JS::Heap<JSObject*> mResultArrayBuffer;
};

#endif
