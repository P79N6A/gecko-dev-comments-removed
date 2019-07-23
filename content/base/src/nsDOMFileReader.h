




































#ifndef nsDOMFileReader_h__
#define nsDOMFileReader_h__

#include "nsISupportsUtils.h"      
#include "nsString.h"              
#include "nsIStreamListener.h"     
#include "nsIScriptContext.h"      
#include "nsIInterfaceRequestor.h" 
#include "nsJSUtils.h"             
#include "nsTArray.h"              
#include "nsIJSNativeInitializer.h"
#include "prtime.h"                
#include "nsITimer.h"              
#include "nsDOMEventTargetHelper.h"
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

#include "nsXMLHttpRequest.h"

class nsDOMFileReader : public nsXHREventTarget,
                        public nsIDOMFileReader,
                        public nsIStreamListener,
                        public nsIInterfaceRequestor,
                        public nsSupportsWeakReference,
                        public nsIJSNativeInitializer,
                        public nsITimerCallback,
                        public nsICharsetDetectionObserver
{
public:
  nsDOMFileReader();
  virtual ~nsDOMFileReader();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMFILEREADER
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMFileReader, nsXHREventTarget)

  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::);
        
  
  NS_DECL_NSISTREAMLISTENER
                               
  
  NS_DECL_NSIREQUESTOBSERVER
                               
  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  NS_DECL_NSITIMERCALLBACK
                               
  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj, 
                        PRUint32 argc, jsval* argv);

  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMNSEVENTTARGET(nsXHREventTarget::)

  
  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf);

  void DispatchProgressEvent(const nsAString& aType);

  nsresult Init();

protected:
  nsresult ReadFileContent(nsIDOMFile *aFile, const nsAString &aCharset, PRUint32 aDataFormat); 
  nsresult GetAsText(const nsAString &aCharset,
                     const char *aFileData, PRUint32 aDataLen, nsAString &aResult);
  nsresult GetAsDataURL(nsIFile *aFile, const char *aFileData, PRUint32 aDataLen, nsAString &aResult); 
  nsresult GuessCharset(const char *aFileData, PRUint32 aDataLen, nsACString &aCharset); 
  nsresult ConvertStream(const char *aFileData, PRUint32 aDataLen, const char *aCharset, nsAString &aResult); 
  void DispatchError(nsresult rv);
  void StartProgressEventTimer();

  void FreeFileData() {
    PR_Free(mFileData);
    mFileData = nsnull;
    mDataLen = 0;
  }

  char *mFileData;
  nsCOMPtr<nsIFile> mFile;
  nsString mCharset;
  PRUint32 mDataLen;
  PRUint32 mDataFormat;

  nsString mResult;
  PRUint16 mReadyState;

  PRBool mProgressEventWasDelayed;
  PRBool mTimerIsActive;
  nsCOMPtr<nsIDOMFileError> mError;

  nsCOMPtr<nsITimer> mProgressNotifier;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIChannel> mChannel;

  PRInt64 mReadTotal;
  PRUint64 mReadTransferred;

  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadEndListener;
};

#endif
