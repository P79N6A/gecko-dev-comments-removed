






































#include <objbase.h>
#include <mlang.h>

#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsNativeCharDetDll.h"
#include "pratom.h"
#include "nsReadableUtils.h"

#include "nsICharsetDetector.h"
#include "nsICharsetDetectionObserver.h"
#include "nsIStringCharsetDetector.h"







static HRESULT DetectCharsetUsingMLang(IMultiLanguage *aMultiLanguage, IMLangConvertCharset *aMLangConvertCharset, 
                                       char* charset, nsDetectionConfident& aConfidence, BYTE* inBuf, UINT* inSize)
{
  if (NULL == charset) {
    return E_FAIL;
  }
  if (*inSize > 4096) {
    return E_FAIL;  
  }
  aConfidence = eNoAnswerYet;

  BYTE outBuf[4096];
  UINT outSize = 0;

  HRESULT hr = aMLangConvertCharset->DoConversion(inBuf, inSize, outBuf, &outSize);
  if (SUCCEEDED(hr)) {
    DWORD dwProperty = 0;
    UINT sourceCP;
    hr = aMLangConvertCharset->GetProperty(&dwProperty);
    if (SUCCEEDED(hr)) {
      if (dwProperty & MLCONVCHARF_AUTODETECT) {
        hr = aMLangConvertCharset->GetSourceCodePage(&sourceCP);
        if (SUCCEEDED(hr)) {
          MIMECPINFO aCodePageInfo;
          hr = aMultiLanguage->GetCodePageInfo(sourceCP, &aCodePageInfo);
          if (SUCCEEDED(hr)) {
            
            nsString aCharset(aCodePageInfo.wszWebCharset);
            char *cstr = ToNewCString(aCharset);
            PL_strcpy(charset, cstr);
            delete [] cstr;
            aConfidence = eSureAnswer;
          }
        }
      }
      else {
        charset[0] = '\0';
        aConfidence = eNoAnswerMatch;
      }
    }
  }
  return hr;
}



class nsNativeDetector : 
      public nsICharsetDetector 
{
public:
  NS_DECL_ISUPPORTS

  nsNativeDetector(PRUint32 aCodePage);
  virtual ~nsNativeDetector();
  NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe);
  NS_IMETHOD Done();
 
private:
  nsICharsetDetectionObserver* mObserver;
  IMultiLanguage *mMultiLanguage;
  IMLangConvertCharset *mMLangConvertCharset;
  PRUint32  mCodePage;
  char mCharset[65];
};

NS_IMPL_ISUPPORTS1(nsNativeDetector, nsICharsetDetector)


nsNativeDetector::nsNativeDetector(PRUint32 aCodePage)
{
  HRESULT hr = CoInitialize(NULL);
  mObserver = nsnull;
  mCodePage = aCodePage;
  mMultiLanguage = NULL;
  mMLangConvertCharset = NULL;
}

nsNativeDetector::~nsNativeDetector()
{
  NS_IF_RELEASE(mObserver);
  if (NULL != mMultiLanguage)
    mMultiLanguage->Release();
  if (NULL != mMLangConvertCharset)
    mMLangConvertCharset->Release();
  CoUninitialize();
}

NS_IMETHODIMP nsNativeDetector::Init(
  nsICharsetDetectionObserver* aObserver)
{
  NS_ASSERTION(mObserver == nsnull , "Init twice");
  if(nsnull == aObserver)
     return NS_ERROR_ILLEGAL_VALUE;

  mObserver = aObserver;

  HRESULT hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IMultiLanguage, (LPVOID *)&mMultiLanguage);
  if (SUCCEEDED(hr)) {
    DWORD dwProperty = 0;
    hr = mMultiLanguage->CreateConvertCharset(mCodePage, 1200, dwProperty, &mMLangConvertCharset);
  }

  return SUCCEEDED(hr) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsNativeDetector::DoIt(
  const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe)
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");

  if((nsnull == aBuf) || (nsnull == oDontFeedMe))
     return NS_ERROR_ILLEGAL_VALUE;

  UINT theSize = (UINT) aLen;
  nsDetectionConfident aConfidence;
  if (SUCCEEDED(DetectCharsetUsingMLang(mMultiLanguage, mMLangConvertCharset, 
                                        mCharset, aConfidence, (BYTE *) aBuf, &theSize))) {
    if (eNoAnswerMatch != aConfidence) {
      mObserver->Notify(mCharset, aConfidence);
    }
  }
  else {
    mObserver->Notify("", eNoAnswerMatch);
  }

  *oDontFeedMe = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP nsNativeDetector::Done()
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
  return NS_OK;
}


class nsNativeStringDetector : 
      public nsIStringCharsetDetector 
{
public:
  NS_DECL_ISUPPORTS

  nsNativeStringDetector(PRUint32 aCodePage);
  virtual ~nsNativeStringDetector();
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                  const char** oCharset, 
                  nsDetectionConfident &oConfident);
protected:
  PRUint32  mCodePage;
  IMultiLanguage *mMultiLanguage;
  char mCharset[65];
};

NS_IMPL_ISUPPORTS1(nsNativeStringDetector, nsIStringCharsetDetector)


nsNativeStringDetector::nsNativeStringDetector(PRUint32 aCodePage)
{
  HRESULT hr = CoInitialize(NULL);
  mCodePage = aCodePage;
  mMultiLanguage = NULL;
}

nsNativeStringDetector::~nsNativeStringDetector()
{
  if (NULL != mMultiLanguage)
    mMultiLanguage->Release();
  CoUninitialize();
}


NS_IMETHODIMP nsNativeStringDetector::DoIt(const char* aBuf, PRUint32 aLen, 
                                           const char** oCharset, 
                                           nsDetectionConfident &oConfident)
{
	HRESULT hr = S_OK;

  oConfident = eNoAnswerMatch;
  *oCharset = "";

  if (NULL == mMultiLanguage) {
    hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IMultiLanguage, (LPVOID *)&mMultiLanguage);
  }

  if (SUCCEEDED(hr)) {
    IMLangConvertCharset *aMLangConvertCharset;
    DWORD dwProperty = 0;
    hr = mMultiLanguage->CreateConvertCharset(mCodePage, 1200, dwProperty, &aMLangConvertCharset);
    if (SUCCEEDED(hr)) {
      UINT theSize = (UINT) aLen;
      nsDetectionConfident aConfidence;

      hr = DetectCharsetUsingMLang(mMultiLanguage, aMLangConvertCharset, 
                                   mCharset, aConfidence, (BYTE *) aBuf, &theSize);
      if (SUCCEEDED(hr)) {
        *oCharset = mCharset;
        oConfident = aConfidence;
      }

      aMLangConvertCharset->Release();
    }
  }

  return SUCCEEDED(hr) ? NS_OK : NS_ERROR_FAILURE;
}


class nsNativeDetectorFactory : public nsIFactory {
   NS_DECL_ISUPPORTS

public:
   nsNativeDetectorFactory(PRUint32 aCodePage, PRBool stringBase) {
     mCodePage = aCodePage;
     mStringBase = stringBase;
   }
   virtual ~nsNativeDetectorFactory() {
   }

   NS_IMETHOD CreateInstance(nsISupports* aDelegate, const nsIID& aIID, void** aResult);
   NS_IMETHOD LockFactory(PRBool aLock);
private:
   PRUint32 mCodePage;
   PRBool mStringBase;
};


NS_IMPL_ISUPPORTS1(nsNativeDetectorFactory, nsIFactory)

NS_IMETHODIMP nsNativeDetectorFactory::CreateInstance(
    nsISupports* aDelegate, const nsIID &aIID, void** aResult)
{
  if(NULL == aResult)
        return NS_ERROR_NULL_POINTER;
  if(NULL != aDelegate)
        return NS_ERROR_NO_AGGREGATION;

  *aResult = NULL;

  nsISupports *inst = nsnull;
  if (mStringBase)
    inst = (nsISupports *) new nsNativeStringDetector(mCodePage);
   else
    inst = (nsISupports *) new nsNativeDetector(mCodePage);
  if(NULL == inst) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(inst);  
  nsresult res =inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst); 

  return res;
}

NS_IMETHODIMP nsNativeDetectorFactory::LockFactory(PRBool aLock)
{
  return NS_OK;
}


nsIFactory* NEW_JA_NATIVEDETECTOR_FACTORY() {
  return new nsNativeDetectorFactory(50932, PR_FALSE);
}
nsIFactory* NEW_JA_STRING_NATIVEDETECTOR_FACTORY() {
  return new nsNativeDetectorFactory(50932, PR_TRUE);
}
nsIFactory* NEW_KO_NATIVEDETECTOR_FACTORY() {
  return new nsNativeDetectorFactory(50949, PR_FALSE);
}
nsIFactory* NEW_KO_STRING_NATIVEDETECTOR_FACTORY() {
  return new nsNativeDetectorFactory(50949, PR_TRUE);
}
