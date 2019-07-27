




#include "nsCrypto.h"
#include "nsNSSComponent.h"
#include "secmod.h"

#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"
#include "nsISaveAsCharset.h"
#include "nsNativeCharsetUtils.h"
#include "nsServiceManagerUtils.h"


NS_INTERFACE_MAP_BEGIN(nsPkcs11)
  NS_INTERFACE_MAP_ENTRY(nsIPKCS11)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPkcs11)
NS_IMPL_RELEASE(nsPkcs11)

nsPkcs11::nsPkcs11()
{
}

nsPkcs11::~nsPkcs11()
{
}


NS_IMETHODIMP
nsPkcs11::DeleteModule(const nsAString& aModuleName)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  nsString errorMessage;

  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  if (aModuleName.IsEmpty()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  NS_ConvertUTF16toUTF8 modName(aModuleName);
  int32_t modType;
  SECStatus srv = SECMOD_DeleteModule(modName.get(), &modType);
  if (srv == SECSuccess) {
    SECMODModule *module = SECMOD_FindModule(modName.get());
    if (module) {
#ifndef MOZ_NO_SMART_CARDS
      nssComponent->ShutdownSmartCardThread(module);
#endif
      SECMOD_DestroyModule(module);
    }
    rv = NS_OK;
  } else {
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}


NS_IMETHODIMP
nsPkcs11::AddModule(const nsAString& aModuleName, 
                    const nsAString& aLibraryFullPath, 
                    int32_t aCryptoMechanismFlags, 
                    int32_t aCipherFlags)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));

  NS_ConvertUTF16toUTF8 moduleName(aModuleName);
  nsCString fullPath;
  
  NS_CopyUnicodeToNative(aLibraryFullPath, fullPath);
  uint32_t mechFlags = SECMOD_PubMechFlagstoInternal(aCryptoMechanismFlags);
  uint32_t cipherFlags = SECMOD_PubCipherFlagstoInternal(aCipherFlags);
  SECStatus srv = SECMOD_AddNewModule(moduleName.get(), fullPath.get(), 
                                      mechFlags, cipherFlags);
  if (srv == SECSuccess) {
    SECMODModule *module = SECMOD_FindModule(moduleName.get());
    if (module) {
#ifndef MOZ_NO_SMART_CARDS
      nssComponent->LaunchSmartCardThread(module);
#endif
      SECMOD_DestroyModule(module);
    }
  }

  
  
  switch (srv) {
  case SECSuccess:
    return NS_OK;
  case SECFailure:
    return NS_ERROR_FAILURE;
  case -2:
    return NS_ERROR_ILLEGAL_VALUE;
  }
  NS_ERROR("Bogus return value, this should never happen");
  return NS_ERROR_FAILURE;
}
