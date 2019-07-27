



#include "nsPKCS11Slot.h"
#include "nsPK11TokenDB.h"

#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "secmod.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

NS_IMPL_ISUPPORTS(nsPKCS11Slot, nsIPKCS11Slot)

nsPKCS11Slot::nsPKCS11Slot(PK11SlotInfo *slot)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  PK11_ReferenceSlot(slot);
  mSlot = slot;
  mSeries = PK11_GetSlotSeries(slot);
  refreshSlotInfo();
}

void
nsPKCS11Slot::refreshSlotInfo()
{
  CK_SLOT_INFO slot_info;
  if (PK11_GetSlotInfo(mSlot, &slot_info) == SECSuccess) {
    
    const char *ccDesc = (const char*)slot_info.slotDescription;
    const nsACString &cDesc = Substring(
      ccDesc, 
      ccDesc+PL_strnlen(ccDesc, sizeof(slot_info.slotDescription)));
    mSlotDesc = NS_ConvertUTF8toUTF16(cDesc);
    mSlotDesc.Trim(" ", false, true);
    
    const char *ccManID = (const char*)slot_info.manufacturerID;
    const nsACString &cManID = Substring(
      ccManID, 
      ccManID+PL_strnlen(ccManID, sizeof(slot_info.manufacturerID)));
    mSlotManID = NS_ConvertUTF8toUTF16(cManID);
    mSlotManID.Trim(" ", false, true);
    
    mSlotHWVersion = EmptyString();
    mSlotHWVersion.AppendInt(slot_info.hardwareVersion.major);
    mSlotHWVersion.Append('.');
    mSlotHWVersion.AppendInt(slot_info.hardwareVersion.minor);
    
    mSlotFWVersion = EmptyString();
    mSlotFWVersion.AppendInt(slot_info.firmwareVersion.major);
    mSlotFWVersion.Append('.');
    mSlotFWVersion.AppendInt(slot_info.firmwareVersion.minor);
  }

}

nsPKCS11Slot::~nsPKCS11Slot()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return;
  }
  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsPKCS11Slot::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsPKCS11Slot::destructorSafeDestroyNSSReference()
{
  if (mSlot) {
    PK11_FreeSlot(mSlot);
    mSlot = nullptr;
  }
}


NS_IMETHODIMP 
nsPKCS11Slot::GetName(char16_t **aName)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  char *csn = PK11_GetSlotName(mSlot);
  if (*csn) {
    *aName = ToNewUnicode(NS_ConvertUTF8toUTF16(csn));
  } else if (PK11_HasRootCerts(mSlot)) {
    
    
    
    *aName = ToNewUnicode(NS_LITERAL_STRING("Root Certificates"));
  } else {
    
    *aName = ToNewUnicode(NS_LITERAL_STRING("Unnamed Slot"));
  }
  if (!*aName) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetDesc(char16_t **aDesc)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshSlotInfo();
  }

  *aDesc = ToNewUnicode(mSlotDesc);
  if (!*aDesc) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetManID(char16_t **aManID)
{
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshSlotInfo();
  }
  *aManID = ToNewUnicode(mSlotManID);
  if (!*aManID) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetHWVersion(char16_t **aHWVersion)
{
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshSlotInfo();
  }
  *aHWVersion = ToNewUnicode(mSlotHWVersion);
  if (!*aHWVersion) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetFWVersion(char16_t **aFWVersion)
{
  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshSlotInfo();
  }
  *aFWVersion = ToNewUnicode(mSlotFWVersion);
  if (!*aFWVersion) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetToken(nsIPK11Token **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIPK11Token> token = new nsPK11Token(mSlot);
  *_retval = token;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Slot::GetTokenName(char16_t **aName)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  if (!PK11_IsPresent(mSlot)) {
    *aName = nullptr;
    return NS_OK;
  }

  if (mSeries != PK11_GetSlotSeries(mSlot)) {
    refreshSlotInfo();
  }


  *aName = ToNewUnicode(NS_ConvertUTF8toUTF16(PK11_GetTokenName(mSlot)));
  if (!*aName) return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMETHODIMP
nsPKCS11Slot::GetStatus(uint32_t *_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  if (PK11_IsDisabled(mSlot))
    *_retval = SLOT_DISABLED;
  else if (!PK11_IsPresent(mSlot))
    *_retval = SLOT_NOT_PRESENT;
  else if (PK11_NeedLogin(mSlot) && PK11_NeedUserInit(mSlot))
    *_retval = SLOT_UNINITIALIZED;
  else if (PK11_NeedLogin(mSlot) && !PK11_IsLoggedIn(mSlot, nullptr))
    *_retval = SLOT_NOT_LOGGED_IN;
  else if (PK11_NeedLogin(mSlot))
    *_retval = SLOT_LOGGED_IN;
  else
    *_retval = SLOT_READY;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsPKCS11Module, nsIPKCS11Module)

nsPKCS11Module::nsPKCS11Module(SECMODModule *module)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  SECMOD_ReferenceModule(module);
  mModule = module;
}

nsPKCS11Module::~nsPKCS11Module()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return;
  }
  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsPKCS11Module::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsPKCS11Module::destructorSafeDestroyNSSReference()
{
  if (mModule) {
    SECMOD_DestroyModule(mModule);
    mModule = nullptr;
  }
}


NS_IMETHODIMP 
nsPKCS11Module::GetName(char16_t **aName)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  *aName = ToNewUnicode(NS_ConvertUTF8toUTF16(mModule->commonName));
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Module::GetLibName(char16_t **aName)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  if ( mModule->dllName ) {
    *aName = ToNewUnicode(NS_ConvertUTF8toUTF16(mModule->dllName));
  } else {
    *aName = nullptr;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Module::FindSlotByName(const char16_t *aName,
                               nsIPKCS11Slot **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  char *asciiname = ToNewUTF8String(nsDependentString(aName));
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting \"%s\"\n", asciiname));
  PK11SlotInfo *slotinfo = nullptr;
  PK11SlotList *slotList = PK11_FindSlotsByNames(mModule->dllName, 
        asciiname , nullptr , false);
  if (!slotList) {
    
    slotList = PK11_FindSlotsByNames(mModule->dllName, 
        nullptr , asciiname , false);
  }
  if (slotList) {
    
    if (slotList->head && slotList->head->slot) {
      slotinfo =  PK11_ReferenceSlot(slotList->head->slot);
    }
    PK11_FreeSlotList(slotList);
  }
  if (!slotinfo) {
    
    if (!asciiname) {
      return NS_ERROR_FAILURE;
    } else if (nsCRT::strcmp(asciiname, "Root Certificates") == 0) {
      slotinfo = PK11_ReferenceSlot(mModule->slots[0]);
    } else {
      
      free(asciiname);
      return NS_ERROR_FAILURE;
    }
  } 
  free(asciiname);
  nsCOMPtr<nsIPKCS11Slot> slot = new nsPKCS11Slot(slotinfo);
  PK11_FreeSlot(slotinfo);
  *_retval = slot;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11Module::ListSlots(nsIEnumerator **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;
  int i;
  
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) return rv;
  



  SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
  SECMOD_GetReadLock(lock);
  for (i=0; i<mModule->slotCount; i++) {
    if (mModule->slots[i]) {
      nsCOMPtr<nsIPKCS11Slot> slot = new nsPKCS11Slot(mModule->slots[i]);
      array->AppendElement(slot);
    }
  }
  SECMOD_ReleaseReadLock(lock);
  rv = array->Enumerate(_retval);
  return rv;
}

NS_IMPL_ISUPPORTS(nsPKCS11ModuleDB, nsIPKCS11ModuleDB, nsICryptoFIPSInfo)

nsPKCS11ModuleDB::nsPKCS11ModuleDB()
{
}

nsPKCS11ModuleDB::~nsPKCS11ModuleDB()
{
}


NS_IMETHODIMP 
nsPKCS11ModuleDB::GetInternal(nsIPKCS11Module **_retval)
{
  nsNSSShutDownPreventionLock locker;
  SECMODModule *nssMod = 
    SECMOD_CreateModule(nullptr, SECMOD_INT_NAME, nullptr, SECMOD_INT_FLAGS);
  nsCOMPtr<nsIPKCS11Module> module = new nsPKCS11Module(nssMod);
  SECMOD_DestroyModule(nssMod);
  *_retval = module;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11ModuleDB::GetInternalFIPS(nsIPKCS11Module **_retval)
{
  nsNSSShutDownPreventionLock locker;
  SECMODModule *nssMod = 
    SECMOD_CreateModule(nullptr, SECMOD_FIPS_NAME, nullptr, SECMOD_FIPS_FLAGS);
  nsCOMPtr<nsIPKCS11Module> module = new nsPKCS11Module(nssMod);
  SECMOD_DestroyModule(nssMod);
  *_retval = module;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11ModuleDB::FindModuleByName(const char16_t *aName,
                                   nsIPKCS11Module **_retval)
{
  nsNSSShutDownPreventionLock locker;
  NS_ConvertUTF16toUTF8 aUtf8Name(aName);
  SECMODModule *mod =
    SECMOD_FindModule(const_cast<char *>(aUtf8Name.get()));
  if (!mod)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPKCS11Module> module = new nsPKCS11Module(mod);
  SECMOD_DestroyModule(mod);
  *_retval = module;
  NS_ADDREF(*_retval);
  return NS_OK;
}





NS_IMETHODIMP 
nsPKCS11ModuleDB::FindSlotByName(const char16_t *aName,
                                 nsIPKCS11Slot **_retval)
{
  nsNSSShutDownPreventionLock locker;
  NS_ConvertUTF16toUTF8 aUtf8Name(aName);
  PK11SlotInfo *slotinfo =
   PK11_FindSlotByName(const_cast<char*>(aUtf8Name.get()));
  if (!slotinfo)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPKCS11Slot> slot = new nsPKCS11Slot(slotinfo);
  PK11_FreeSlot(slotinfo);
  *_retval = slot;
  NS_ADDREF(*_retval);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKCS11ModuleDB::ListModules(nsIEnumerator **_retval)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) return rv;
  
  SECMODModuleList *list = SECMOD_GetDefaultModuleList();
  
  SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
  SECMOD_GetReadLock(lock);
  while (list) {
    nsCOMPtr<nsIPKCS11Module> module = new nsPKCS11Module(list->module);
    array->AppendElement(module);
    list = list->next;
  }
  
  list = SECMOD_GetDeadModuleList();
  while (list) {
    nsCOMPtr<nsIPKCS11Module> module = new nsPKCS11Module(list->module);
    array->AppendElement(module);
    list = list->next;
  }
  SECMOD_ReleaseReadLock(lock);
  rv = array->Enumerate(_retval);
  return rv;
}

NS_IMETHODIMP nsPKCS11ModuleDB::GetCanToggleFIPS(bool *aCanToggleFIPS)
{
  nsNSSShutDownPreventionLock locker;
  *aCanToggleFIPS = SECMOD_CanDeleteInternalModule();
  return NS_OK;
}



NS_IMETHODIMP nsPKCS11ModuleDB::ToggleFIPSMode()
{
  nsNSSShutDownPreventionLock locker;
  
  
  
  
  SECMODModule *internal;

  
  
  
  internal = SECMOD_GetInternalModule();
  if (!internal)
    return NS_ERROR_FAILURE;

  SECStatus srv = SECMOD_DeleteInternalModule(internal->commonName);
  if (srv != SECSuccess)
    return NS_ERROR_FAILURE;

  return NS_OK;
}


NS_IMETHODIMP nsPKCS11ModuleDB::GetIsFIPSEnabled(bool *aIsFIPSEnabled)
{
  nsNSSShutDownPreventionLock locker;
  *aIsFIPSEnabled = PK11_IsFIPS();
  return NS_OK;
}

NS_IMETHODIMP nsPKCS11ModuleDB::GetIsFIPSModeActive(bool *aIsFIPSModeActive)
{
  return GetIsFIPSEnabled(aIsFIPSModeActive);
}
