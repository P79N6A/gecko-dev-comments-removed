





































#include "nsISupports.h"
#include "nsIPK11TokenDB.h"
#include "prerror.h"
#include "secerr.h"
#include "nsReadableUtils.h"
#include "nsNSSComponent.h"

#include "nsPK11TokenDB.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

NS_IMPL_ISUPPORTS1(nsPK11Token, nsIPK11Token)

nsPK11Token::nsPK11Token(PK11SlotInfo *slot)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  PK11_ReferenceSlot(slot);
  mSlot = slot;
  mSeries = PK11_GetSlotSeries(slot);

  refreshTokenInfo();
  mUIContext = new PipUIContext();
}

void  
nsPK11Token::refreshTokenInfo()
{
  mTokenName = NS_ConvertUTF8toUTF16(PK11_GetTokenName(mSlot));

  SECStatus srv;

  CK_TOKEN_INFO tok_info;
  srv = PK11_GetTokenInfo(mSlot, &tok_info);
  if (srv == SECSuccess) {
    

    const char *ccLabel = (const char*)tok_info.label;
    const nsACString &cLabel = Substring(
      ccLabel, 
      ccLabel+PL_strnlen(ccLabel, sizeof(tok_info.label)));
    mTokenLabel = NS_ConvertUTF8toUTF16(cLabel);
    mTokenLabel.Trim(" ", PR_FALSE, PR_TRUE);

    
    const char *ccManID = (const char*)tok_info.manufacturerID;
    const nsACString &cManID = Substring(
      ccManID, 
      ccManID+PL_strnlen(ccManID, sizeof(tok_info.manufacturerID)));
    mTokenManID = NS_ConvertUTF8toUTF16(cManID);
    mTokenManID.Trim(" ", PR_FALSE, PR_TRUE);

    
    mTokenHWVersion.AppendInt(tok_info.hardwareVersion.major);
    mTokenHWVersion.AppendLiteral(".");
    mTokenHWVersion.AppendInt(tok_info.hardwareVersion.minor);
    
    mTokenFWVersion.AppendInt(tok_info.firmwareVersion.major);
    mTokenFWVersion.AppendLiteral(".");
    mTokenFWVersion.AppendInt(tok_info.firmwareVersion.minor);
    
    const char *ccSerial = (const char*)tok_info.serialNumber;
    const nsACString &cSerial = Substring(
      ccSerial, 
      ccSerial+PL_strnlen(ccSerial, sizeof(tok_info.serialNumber)));
    mTokenSerialNum = NS_ConvertUTF8toUTF16(cSerial);
    mTokenSerialNum.Trim(" ", PR_FALSE, PR_TRUE);
  }

}

nsPK11Token::~nsPK11Token()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsPK11Token::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsPK11Token::destructorSafeDestroyNSSReference()
{
  if (isAlreadyShutDown())
    return;

  if (mSlot) {
    PK11_FreeSlot(mSlot);
    mSlot = nsnull;
  }
}


NS_IMETHODIMP nsPK11Token::GetTokenName(PRUnichar * *aTokenName)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokenName = ToNewUnicode(mTokenName);
  if (!*aTokenName) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetTokenLabel(PRUnichar **aTokLabel)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokLabel = ToNewUnicode(mTokenLabel);
  if (!*aTokLabel) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetTokenManID(PRUnichar **aTokManID)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokManID = ToNewUnicode(mTokenManID);
  if (!*aTokManID) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetTokenHWVersion(PRUnichar **aTokHWVersion)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokHWVersion = ToNewUnicode(mTokenHWVersion);
  if (!*aTokHWVersion) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetTokenFWVersion(PRUnichar **aTokFWVersion)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokFWVersion = ToNewUnicode(mTokenFWVersion);
  if (!*aTokFWVersion) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetTokenSerialNumber(PRUnichar **aTokSerialNum)
{
  
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshTokenInfo();
  }
  *aTokSerialNum = ToNewUnicode(mTokenSerialNum);
  if (!*aTokSerialNum) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::IsLoggedIn(PRBool *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  *_retval = PK11_IsLoggedIn(mSlot, 0);

  return rv;
}


NS_IMETHODIMP 
nsPK11Token::Login(PRBool force)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv;
  SECStatus srv;
  PRBool test;
  rv = this->NeedsLogin(&test);
  if (NS_FAILED(rv)) return rv;
  if (test && force) {
    rv = this->LogoutSimple();
    if (NS_FAILED(rv)) return rv;
  }
  rv = setPassword(mSlot, mUIContext);
  if (NS_FAILED(rv)) return rv;
  srv = PK11_Authenticate(mSlot, PR_TRUE, mUIContext);
  return (srv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsPK11Token::LogoutSimple()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  
  
  PK11_Logout(mSlot);
  return NS_OK;
}

NS_IMETHODIMP nsPK11Token::LogoutAndDropAuthenticatedResources()
{
  nsresult rv = LogoutSimple();

  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  return nssComponent->LogoutAuthenticatedPK11();
}


NS_IMETHODIMP nsPK11Token::Reset()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  PK11_ResetToken(mSlot, 0);
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetMinimumPasswordLength(PRInt32 *aMinimumPasswordLength)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  *aMinimumPasswordLength = PK11_GetMinimumPwdLength(mSlot);

  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::GetNeedsUserInit(PRBool *aNeedsUserInit)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  *aNeedsUserInit = PK11_NeedUserInit(mSlot);
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::CheckPassword(const PRUnichar *password, PRBool *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  SECStatus srv;
  PRInt32 prerr;
  NS_ConvertUTF16toUTF8 aUtf8Password(password);
  srv = PK11_CheckUserPassword(mSlot, 
                  const_cast<char *>(aUtf8Password.get()));
  if (srv != SECSuccess) {
    *_retval =  PR_FALSE;
    prerr = PR_GetError();
    if (prerr != SEC_ERROR_BAD_PASSWORD) {
      
      return NS_ERROR_FAILURE;
    }
  } else {
    *_retval =  PR_TRUE;
  }
  return NS_OK;
}


NS_IMETHODIMP nsPK11Token::InitPassword(const PRUnichar *initialPassword)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

    nsresult rv = NS_OK;
    SECStatus status;

    NS_ConvertUTF16toUTF8 aUtf8InitialPassword(initialPassword);
    status = PK11_InitPin(mSlot, "", const_cast<char*>(aUtf8InitialPassword.get()));
    if (status == SECFailure) { rv = NS_ERROR_FAILURE; goto done; }

done:
    return rv;
}


NS_IMETHODIMP 
nsPK11Token::GetAskPasswordTimes(PRInt32 *rvAskTimes)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

    int askTimes, askTimeout;
    PK11_GetSlotPWValues(mSlot, &askTimes, &askTimeout);
    *rvAskTimes = askTimes;
    return NS_OK;
}


NS_IMETHODIMP 
nsPK11Token::GetAskPasswordTimeout(PRInt32 *rvAskTimeout)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

    int askTimes, askTimeout;
    PK11_GetSlotPWValues(mSlot, &askTimes, &askTimeout);
    *rvAskTimeout = askTimeout;
    return NS_OK;
}




NS_IMETHODIMP 
nsPK11Token::SetAskPasswordDefaults(const PRInt32 askTimes,
                                    const PRInt32 askTimeout)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

    PK11_SetSlotPWValues(mSlot, askTimes, askTimeout);
    return NS_OK;
}


NS_IMETHODIMP nsPK11Token::ChangePassword(const PRUnichar *oldPassword, const PRUnichar *newPassword)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  SECStatus rv;
  NS_ConvertUTF16toUTF8 aUtf8OldPassword(oldPassword);
  NS_ConvertUTF16toUTF8 aUtf8NewPassword(newPassword);

  rv = PK11_ChangePW(mSlot, 
         (oldPassword != NULL ? const_cast<char *>(aUtf8OldPassword.get()) : NULL), 
         (newPassword != NULL ? const_cast<char *>(aUtf8NewPassword.get()) : NULL));
  return (rv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsPK11Token::IsHardwareToken(PRBool *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  *_retval = PK11_IsHW(mSlot);

  return rv;
}


NS_IMETHODIMP nsPK11Token::NeedsLogin(PRBool *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  *_retval = PK11_NeedLogin(mSlot);

  return rv;
}


NS_IMETHODIMP nsPK11Token::IsFriendly(PRBool *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  *_retval = PK11_IsFriendly(mSlot);

  return rv;
}



NS_IMPL_ISUPPORTS1(nsPK11TokenDB, nsIPK11TokenDB)

nsPK11TokenDB::nsPK11TokenDB()
{
  
}

nsPK11TokenDB::~nsPK11TokenDB()
{
  
}


NS_IMETHODIMP nsPK11TokenDB::GetInternalKeyToken(nsIPK11Token **_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  PK11SlotInfo *slot = 0;
  nsCOMPtr<nsIPK11Token> token;

  slot = PK11_GetInternalKeySlot();
  if (!slot) { rv = NS_ERROR_FAILURE; goto done; }

  token = new nsPK11Token(slot);
  if (!token) { rv = NS_ERROR_OUT_OF_MEMORY; goto done; }

  *_retval = token;
  NS_ADDREF(*_retval);

done:
  if (slot) PK11_FreeSlot(slot);
  return rv;
}


NS_IMETHODIMP nsPK11TokenDB::
FindTokenByName(const PRUnichar* tokenName, nsIPK11Token **_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  PK11SlotInfo *slot = 0;
  NS_ConvertUTF16toUTF8 aUtf8TokenName(tokenName);
  slot = PK11_FindSlotByName(const_cast<char*>(aUtf8TokenName.get()));
  if (!slot) { rv = NS_ERROR_FAILURE; goto done; }

  *_retval = new nsPK11Token(slot);
  if (!*_retval) { rv = NS_ERROR_OUT_OF_MEMORY; goto done; }

  NS_ADDREF(*_retval);

done:
  if (slot) PK11_FreeSlot(slot);
  return rv;
}


NS_IMETHODIMP nsPK11TokenDB::ListTokens(nsIEnumerator* *_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  nsCOMPtr<nsISupportsArray> array;
  PK11SlotList *list = 0;
  PK11SlotListElement *le;

  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) { goto done; }

  


  list = PK11_GetAllTokens(CKM_INVALID_MECHANISM, PR_FALSE, PR_FALSE, 0);
  if (!list) { rv = NS_ERROR_FAILURE; goto done; }

  for (le = PK11_GetFirstSafe(list); le; le = PK11_GetNextSafe(list, le, PR_FALSE)) {
    nsCOMPtr<nsIPK11Token> token = new nsPK11Token(le->slot);

    array->AppendElement(token);
  }

  rv = array->Enumerate(_retval);

done:
  if (list) PK11_FreeSlotList(list);
  return rv;
}

