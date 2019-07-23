





































#include "nsPrintSettingsX.h"
#include "nsIPrintSessionX.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"

#include "plbase64.h"
#include "prmem.h"
#include "prnetdb.h"



struct FrozenHandle {
  PRUint32 size;
  char     data[0];
};


#define PRINTING_PREF_BRANCH            "print."
#define MAC_OS_X_PAGE_SETUP_PREFNAME    "macosx.pagesetup-2"







class StHandleOwner
{
public:
  StHandleOwner(Handle inHandle)
    : mHandle(inHandle)
  {
  }

  ~StHandleOwner()
  {
    if (mHandle)
      ::DisposeHandle(mHandle);
  }

  Handle GetHandle() { return mHandle; }

  void   ClearHandle(Boolean disposeIt = false)
  {
    if (disposeIt)
      ::DisposeHandle(mHandle);

    mHandle = nsnull;
  }

protected:

  Handle            mHandle;
};






class StHandleLocker
{
public:

  StHandleLocker(Handle theHandle)
    :	mHandle(theHandle)
  {
    if (mHandle)
      {
        mOldHandleState = ::HGetState(mHandle);
        ::HLock(mHandle);
      }										  
  }

  ~StHandleLocker()
  {
    if (mHandle)
      ::HSetState(mHandle, mOldHandleState);
  }

protected:

  Handle          mHandle;
  SInt8           mOldHandleState;
};


NS_IMPL_ISUPPORTS_INHERITED1(nsPrintSettingsX, 
                             nsPrintSettings, 
                             nsIPrintSettingsX)



nsPrintSettingsX::nsPrintSettingsX() :
  mPageFormat(kPMNoPageFormat),
  mPrintSettings(kPMNoPrintSettings)
{
}



nsPrintSettingsX::nsPrintSettingsX(const nsPrintSettingsX& src) :
  mPageFormat(kPMNoPageFormat),
  mPrintSettings(kPMNoPrintSettings)
{
  *this = src;
}



nsPrintSettingsX::~nsPrintSettingsX()
{
  if (mPageFormat != kPMNoPageFormat) {
    ::PMRelease(mPageFormat);
    mPageFormat = kPMNoPageFormat;
  }
  if (mPrintSettings != kPMNoPrintSettings) {
    ::PMRelease(mPrintSettings);
    mPrintSettings = kPMNoPrintSettings;
  }
}



nsPrintSettingsX& nsPrintSettingsX::operator=(const nsPrintSettingsX& rhs)
{
  if (this == &rhs) {
    return *this;
  }
  
  nsPrintSettings::operator=(rhs);

  OSStatus status;
   
  if (mPageFormat != kPMNoPageFormat) {
    ::PMRelease(mPageFormat);
    mPageFormat = kPMNoPageFormat;
  }
  if (rhs.mPageFormat != kPMNoPageFormat) {
    PMPageFormat pageFormat;
    status = ::PMCreatePageFormat(&pageFormat);
    if (status == noErr) {
      status = ::PMCopyPageFormat(rhs.mPageFormat, pageFormat);
      if (status == noErr)
        mPageFormat = pageFormat;
      else
        ::PMRelease(pageFormat);
    }
  }
  
  if (mPrintSettings != kPMNoPrintSettings) {
    ::PMRelease(mPrintSettings);
    mPrintSettings = kPMNoPrintSettings;
  }
  if (rhs.mPrintSettings != kPMNoPrintSettings) {
    PMPrintSettings    printSettings;
    status = ::PMCreatePrintSettings(&printSettings);
    if (status == noErr) {
      status = ::PMCopyPrintSettings(rhs.mPrintSettings, printSettings);
      if (status == noErr)
        mPrintSettings = printSettings;
      else
        ::PMRelease(printSettings);
    }
  }

  return *this;
}



nsresult nsPrintSettingsX::Init()
{
  OSStatus status;

  PMPrintSession printSession = NULL;
  status = ::PMCreateSession(&printSession);
  
  if (status == noErr) {
    
    status = CreateDefaultPageFormat(printSession, mPageFormat);

    
    if (status == noErr) {
      status = CreateDefaultPrintSettings(printSession, mPrintSettings);
    }
    OSStatus tempStatus = ::PMRelease(printSession);
    if (status == noErr)
      status = tempStatus;
  }
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::GetNativePrintSession(PMPrintSession *aNativePrintSession)
{
   NS_ENSURE_ARG_POINTER(aNativePrintSession);
   *aNativePrintSession = nsnull;
   
   nsCOMPtr<nsIPrintSession> printSession;
   GetPrintSession(getter_AddRefs(printSession));
   if (!printSession)
    return NS_ERROR_FAILURE;
   nsCOMPtr<nsIPrintSessionX> printSessionX(do_QueryInterface(printSession));
   if (!printSession)
    return NS_ERROR_FAILURE;

   return printSessionX->GetNativeSession(aNativePrintSession);
}



NS_IMETHODIMP nsPrintSettingsX::GetPMPageFormat(PMPageFormat *aPMPageFormat)
{
  NS_ENSURE_ARG_POINTER(aPMPageFormat);
  *aPMPageFormat = kPMNoPageFormat;
  NS_ENSURE_STATE(mPageFormat != kPMNoPageFormat);
  
  *aPMPageFormat = mPageFormat;
  OSStatus status = noErr;
  
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::SetPMPageFormat(PMPageFormat aPMPageFormat)
{
  NS_ENSURE_ARG(aPMPageFormat);
  
  OSStatus status = ::PMRetain(aPMPageFormat);
  if (status == noErr) {
    if (mPageFormat)
      status = ::PMRelease(mPageFormat);
    mPageFormat = aPMPageFormat;
  }        
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::GetPMPrintSettings(PMPrintSettings *aPMPrintSettings)
{
  NS_ENSURE_ARG_POINTER(aPMPrintSettings);
  *aPMPrintSettings = kPMNoPrintSettings;
  NS_ENSURE_STATE(mPrintSettings != kPMNoPrintSettings);
  
  *aPMPrintSettings = mPrintSettings;
  OSStatus status = noErr;
  
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::SetPMPrintSettings(PMPrintSettings aPMPrintSettings)
{
  NS_ENSURE_ARG(aPMPrintSettings);
  
  OSStatus status = ::PMRetain(aPMPrintSettings);
  if (status == noErr) {
    if (mPrintSettings)
      status = ::PMRelease(mPrintSettings);
    mPrintSettings = aPMPrintSettings;
  }        
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::ReadPageFormatFromPrefs()
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(PRINTING_PREF_BRANCH, getter_AddRefs(prefBranch));
  if (NS_FAILED(rv))
    return rv;
      
  nsXPIDLCString  encodedData;
  rv = prefBranch->GetCharPref(MAC_OS_X_PAGE_SETUP_PREFNAME, getter_Copies(encodedData));
  if (NS_FAILED(rv))
    return rv;

  
  PRInt32 encodedDataLen = encodedData.Length();
  FrozenHandle* frozenHandle =
   (FrozenHandle*)::PL_Base64Decode(encodedData.get(), encodedDataLen, nsnull);
  if (!frozenHandle)
    return NS_ERROR_FAILURE;

  PRUint32 handleSize = PR_ntohl(frozenHandle->size);

  
  
  
  PRUint32 maximumDataSize = (encodedDataLen * 3) / 4 - sizeof(FrozenHandle);
  PRUint32 minimumDataSize = maximumDataSize - 2;
  if (handleSize > maximumDataSize || handleSize < minimumDataSize) {
    PR_Free(frozenHandle);
    return NS_ERROR_FAILURE;
  }

  Handle    decodedDataHandle = nsnull;
  OSErr err = ::PtrToHand(frozenHandle->data, &decodedDataHandle, handleSize);
  PR_Free(frozenHandle);
  if (err != noErr)
    return NS_ERROR_OUT_OF_MEMORY;

  StHandleOwner   handleOwner(decodedDataHandle);  

  OSStatus      status;
  PMPageFormat  newPageFormat = kPMNoPageFormat;
  
  status = ::PMCreatePageFormat(&newPageFormat);
  if (status == noErr) { 
    status = ::PMUnflattenPageFormat(decodedDataHandle, &newPageFormat);
    if (status == noErr) {
      if (mPageFormat)
        status = ::PMRelease(mPageFormat);
      mPageFormat = newPageFormat; 
    }
  }
  return (status == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsPrintSettingsX::WritePageFormatToPrefs()
{
  if (mPageFormat == kPMNoPageFormat)
    return NS_ERROR_NOT_INITIALIZED;
    
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(PRINTING_PREF_BRANCH, getter_AddRefs(prefBranch));
  if (NS_FAILED(rv))
    return rv;

  Handle    pageFormatHandle = nsnull;
  OSStatus  err = ::PMFlattenPageFormat(mPageFormat, &pageFormatHandle);
  if (err != noErr)
    return NS_ERROR_FAILURE;
    
  StHandleOwner   handleOwner(pageFormatHandle);
  StHandleLocker  handleLocker(pageFormatHandle);

  
  
  
  
  
  PRUint32 dataSize = ::GetHandleSize(pageFormatHandle);
  PRUint32 frozenDataSize = sizeof(FrozenHandle) + dataSize;
  FrozenHandle* frozenHandle = (FrozenHandle*)PR_Malloc(frozenDataSize);
  if (!frozenHandle)
    return NS_ERROR_OUT_OF_MEMORY;

  frozenHandle->size = PR_htonl(dataSize);
  memcpy(&frozenHandle->data, *pageFormatHandle, dataSize);

  nsXPIDLCString  encodedData;
  encodedData.Adopt(::PL_Base64Encode((char*)frozenHandle, frozenDataSize,
                    nsnull));
  PR_Free(frozenHandle);
  if (!encodedData.get())
    return NS_ERROR_OUT_OF_MEMORY;

  return prefBranch->SetCharPref(MAC_OS_X_PAGE_SETUP_PREFNAME, encodedData);
}


nsresult nsPrintSettingsX::_Clone(nsIPrintSettings **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;
  
  nsPrintSettingsX *newSettings = new nsPrintSettingsX(*this);
  if (!newSettings)
    return NS_ERROR_FAILURE;
  *_retval = newSettings;
  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP nsPrintSettingsX::_Assign(nsIPrintSettings *aPS)
{
  nsPrintSettingsX *printSettingsX = NS_STATIC_CAST(nsPrintSettingsX*, aPS);
  if (!printSettingsX)
    return NS_ERROR_UNEXPECTED;
  *this = *printSettingsX;
  return NS_OK;
}


OSStatus nsPrintSettingsX::CreateDefaultPageFormat(PMPrintSession aSession, PMPageFormat& outFormat)
{
  OSStatus status;
  PMPageFormat pageFormat;
  
  outFormat = kPMNoPageFormat;
  status = ::PMCreatePageFormat(&pageFormat);
    if (status == noErr && pageFormat != kPMNoPageFormat) {
      status = ::PMSessionDefaultPageFormat(aSession, pageFormat);
    if (status == noErr) {
      outFormat = pageFormat;
      return NS_OK;
    }
  }
  return status;
}
  


OSStatus nsPrintSettingsX::CreateDefaultPrintSettings(PMPrintSession aSession, PMPrintSettings& outSettings)
{
  OSStatus status;
  PMPrintSettings printSettings;
  
  outSettings = kPMNoPrintSettings;
  status = ::PMCreatePrintSettings(&printSettings);
  if (status == noErr && printSettings != kPMNoPrintSettings) {
    status = ::PMSessionDefaultPrintSettings(aSession, printSettings);
    if (status == noErr) {
      outSettings = printSettings;
      return noErr;
    }
  }
  return status;  
}

