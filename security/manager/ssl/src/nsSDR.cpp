






































#include "stdlib.h"
#include "plstr.h"
#include "plbase64.h"

#include "nsMemory.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsProxiedService.h"
#include "nsITokenPasswordDialogs.h"

#include "nsISecretDecoderRing.h"
#include "nsSDR.h"
#include "nsNSSComponent.h"
#include "nsNSSShutDown.h"

#include "pk11func.h"
#include "pk11sdr.h" 

#include "ssl.h" 

#include "nsNSSCleaner.h"
NSSCleanupAutoPtrClass(PK11SlotInfo, PK11_FreeSlot)





class nsSDRContext : public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  nsSDRContext();
  virtual ~nsSDRContext();

};

NS_IMPL_ISUPPORTS1(nsSDRContext, nsIInterfaceRequestor)

nsSDRContext::nsSDRContext()
{
}

nsSDRContext::~nsSDRContext()
{
}


NS_IMETHODIMP nsSDRContext::GetInterface(const nsIID & uuid, void * *result)
{
  if (uuid.Equals(NS_GET_IID(nsIPrompt))) {
    nsCOMPtr<nsIPrompt> prompter;
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (wwatch) {
      wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
      if (prompter) {
        nsCOMPtr<nsIPrompt> proxyPrompt;
        NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                             NS_GET_IID(nsIPrompt),
                             prompter, NS_PROXY_SYNC,
                             getter_AddRefs(proxyPrompt));
        if (!proxyPrompt) return NS_ERROR_FAILURE;
        *result = proxyPrompt;
        NS_ADDREF((nsIPrompt*)*result);
      }
    }
  } else {
    return NS_ERROR_NO_INTERFACE;
  }

  return NS_OK;
}



NS_IMPL_ISUPPORTS2(nsSecretDecoderRing, nsISecretDecoderRing, nsISecretDecoderRingConfig)


nsSecretDecoderRing::nsSecretDecoderRing()
{
  
}


nsSecretDecoderRing::~nsSecretDecoderRing()
{
}


NS_IMETHODIMP nsSecretDecoderRing::
Encrypt(unsigned char * data, PRInt32 dataLen, unsigned char * *result, PRInt32 *_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  PK11SlotInfo *slot = 0;
  PK11SlotInfoCleaner tmpSlotCleaner(slot);
  SECItem keyid;
  SECItem request;
  SECItem reply;
  SECStatus s;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsSDRContext();

  slot = PK11_GetInternalKeySlot();
  if (!slot) { rv = NS_ERROR_NOT_AVAILABLE; goto loser; }

  
  rv = setPassword(slot, ctx);
  if (NS_FAILED(rv))
    goto loser;

  s = PK11_Authenticate(slot, PR_TRUE, ctx);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto loser; }

  
  keyid.data = 0;
  keyid.len = 0;
  request.data = data;
  request.len = dataLen;
  reply.data = 0;
  reply.len = 0;
  s= PK11SDR_Encrypt(&keyid, &request, &reply, ctx);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto loser; }

  *result = reply.data;
  *_retval = reply.len;

loser:
  return rv;
}


NS_IMETHODIMP nsSecretDecoderRing::
Decrypt(unsigned char * data, PRInt32 dataLen, unsigned char * *result, PRInt32 *_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  PK11SlotInfo *slot = 0;
  PK11SlotInfoCleaner tmpSlotCleaner(slot);
  SECStatus s;
  SECItem request;
  SECItem reply;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsSDRContext();

  *result = 0;
  *_retval = 0;

  
  slot = PK11_GetInternalKeySlot();
  if (!slot) { rv = NS_ERROR_NOT_AVAILABLE; goto loser; }

  
  if (PK11_Authenticate(slot, PR_TRUE, ctx) != SECSuccess)
  {
    rv = NS_ERROR_NOT_AVAILABLE;
    goto loser;
  }

  request.data = data;
  request.len = dataLen;
  reply.data = 0;
  reply.len = 0;
  s = PK11SDR_Decrypt(&request, &reply, ctx);
  if (s != SECSuccess) { rv = NS_ERROR_FAILURE; goto loser; }

  *result = reply.data;
  *_retval = reply.len;

loser:
  return rv;
}


NS_IMETHODIMP nsSecretDecoderRing::
EncryptString(const char *text, char **_retval)
{
    nsNSSShutDownPreventionLock locker;
    nsresult rv = NS_OK;
    unsigned char *encrypted = 0;
    PRInt32 eLen;

    if (text == nsnull || _retval == nsnull) {
        rv = NS_ERROR_INVALID_POINTER;
        goto loser;
    }

    rv = Encrypt((unsigned char *)text, PL_strlen(text), &encrypted, &eLen);
    if (rv != NS_OK) { goto loser; }

    rv = encode(encrypted, eLen, _retval);

loser:
    if (encrypted) nsMemory::Free(encrypted);

    return rv;
}


NS_IMETHODIMP nsSecretDecoderRing::
DecryptString(const char *crypt, char **_retval)
{
    nsNSSShutDownPreventionLock locker;
    nsresult rv = NS_OK;
    char *r = 0;
    unsigned char *decoded = 0;
    PRInt32 decodedLen;
    unsigned char *decrypted = 0;
    PRInt32 decryptedLen;

    if (crypt == nsnull || _retval == nsnull) {
      rv = NS_ERROR_INVALID_POINTER;
      goto loser;
    }

    rv = decode(crypt, &decoded, &decodedLen);
    if (rv != NS_OK) goto loser;

    rv = Decrypt(decoded, decodedLen, &decrypted, &decryptedLen);
    if (rv != NS_OK) goto loser;

    
    r = (char *)nsMemory::Alloc(decryptedLen+1);
    if (!r) { rv = NS_ERROR_OUT_OF_MEMORY; goto loser; }

    memcpy(r, decrypted, decryptedLen);
    r[decryptedLen] = 0;

    *_retval = r;
    r = 0;

loser:
    if (decrypted) nsMemory::Free(decrypted);
    if (decoded) nsMemory::Free(decoded);
 
    return rv;
}


NS_IMETHODIMP nsSecretDecoderRing::
ChangePassword()
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  PK11SlotInfo *slot;

  slot = PK11_GetInternalKeySlot();
  if (!slot) return NS_ERROR_NOT_AVAILABLE;

  
  NS_ConvertUTF8toUTF16 tokenName(PK11_GetTokenName(slot));

  PK11_FreeSlot(slot);

  
  nsCOMPtr<nsITokenPasswordDialogs> dialogs;

  rv = getNSSDialogs(getter_AddRefs(dialogs),
           NS_GET_IID(nsITokenPasswordDialogs),
           NS_TOKENPASSWORDSDIALOG_CONTRACTID);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsSDRContext();
  PRBool canceled;

  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      rv = dialogs->SetPassword(ctx, tokenName.get(), &canceled);
    }
  }

  


  return rv;
}

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

NS_IMETHODIMP nsSecretDecoderRing::
Logout()
{
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  {
    nsNSSShutDownPreventionLock locker;
    PK11_LogoutAll();
    SSL_ClearSessionCache();
  }

  return NS_OK;
}

NS_IMETHODIMP nsSecretDecoderRing::
LogoutAndTeardown()
{
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  {
    nsNSSShutDownPreventionLock locker;
    PK11_LogoutAll();
    SSL_ClearSessionCache();
  }

  return nssComponent->LogoutAuthenticatedPK11();
}


NS_IMETHODIMP nsSecretDecoderRing::
SetWindow(nsISupports *w)
{
  return NS_OK;
}



nsresult nsSecretDecoderRing::
encode(const unsigned char *data, PRInt32 dataLen, char **_retval)
{
    nsresult rv = NS_OK;

    *_retval = PL_Base64Encode((const char *)data, dataLen, NULL);
    if (!*_retval) { rv = NS_ERROR_OUT_OF_MEMORY; goto loser; }

loser:
    return rv;
}

nsresult nsSecretDecoderRing::
decode(const char *data, unsigned char **result, PRInt32 * _retval)
{
    nsresult rv = NS_OK;
    PRUint32 len = PL_strlen(data);
    int adjust = 0;

    
    if (data[len-1] == '=') {
      adjust++;
      if (data[len-2] == '=') adjust++;
    }

    *result = (unsigned char *)PL_Base64Decode(data, len, NULL);
    if (!*result) { rv = NS_ERROR_ILLEGAL_VALUE; goto loser; }

    *_retval = (len*3)/4 - adjust;

loser:
    return rv;
}
