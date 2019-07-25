




































#ifndef nsDOMFileReader_h__
#define nsDOMFileReader_h__

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
#include "nsICharsetDetector.h"
#include "nsICharsetDetectionObserver.h"

#include "nsIDOMFile.h"
#include "nsIDOMFileReader.h"
#include "nsIDOMFileList.h"
#include "nsIDOMFileError.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"
#include "nsIStreamLoader.h"
#include "nsIChannel.h"
#include "prmem.h"

#include "FileIOObject.h"

class nsDOMFileReader : public mozilla::dom::FileIOObject,
                        public nsIDOMFileReader,
                        public nsIInterfaceRequestor,
                        public nsSupportsWeakReference,
                        public nsIJSNativeInitializer,
                        public nsICharsetDetectionObserver
{
public:
  nsDOMFileReader();
  virtual ~nsDOMFileReader();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMFILEREADER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  
  NS_DECL_NSIINTERFACEREQUESTOR

  NS_DECL_EVENT_HANDLER(load)
  NS_DECL_EVENT_HANDLER(loadend)
  NS_DECL_EVENT_HANDLER(loadstart)

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj, 
                        PRUint32 argc, jsval* argv);

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);

  
  NS_IMETHOD DoAbort(nsAString& aEvent);
  NS_IMETHOD DoOnStopRequest(nsIRequest* aRequest, nsISupports* aContext,
                             nsresult aStatus, nsAString& aSuccessEvent,
                             nsAString& aTerminationEvent);
  NS_IMETHOD DoOnDataAvailable(nsIRequest* aRequest, nsISupports* aContext,
                               nsIInputStream* aInputStream, PRUint32 aOffset,
                               PRUint32 aCount);

  nsresult Init();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMFileReader,
                                                         FileIOObject)
  void RootResultArrayBuffer();

protected:
  enum eDataFormat {
    FILE_AS_ARRAYBUFFER,
    FILE_AS_BINARY,
    FILE_AS_TEXT,
    FILE_AS_DATAURL
  };

  nsresult ReadFileContent(JSContext* aCx, nsIDOMBlob *aFile, const nsAString &aCharset, eDataFormat aDataFormat); 
  nsresult GetAsText(const nsACString &aCharset,
                     const char *aFileData, PRUint32 aDataLen, nsAString &aResult);
  nsresult GetAsDataURL(nsIDOMBlob *aFile, const char *aFileData, PRUint32 aDataLen, nsAString &aResult); 
  nsresult GuessCharset(const char *aFileData, PRUint32 aDataLen, nsACString &aCharset); 
  nsresult ConvertStream(const char *aFileData, PRUint32 aDataLen, const char *aCharset, nsAString &aResult); 

  void FreeFileData() {
    PR_Free(mFileData);
    mFileData = nsnull;
    mDataLen = 0;
  }

  char *mFileData;
  nsCOMPtr<nsIDOMBlob> mFile;
  nsCString mCharset;
  PRUint32 mDataLen;

  eDataFormat mDataFormat;

  nsString mResult;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  
  JSObject* mResultArrayBuffer;
};

#endif
