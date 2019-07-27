





#include "nsCrypto.h"

#include "nsNSSComponent.h"
#include "nsNativeCharsetUtils.h"
#include "nsServiceManagerUtils.h"
#include "ScopedNSSTypes.h"


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
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return;
  }
  shutdown(calledFromObject);
}


NS_IMETHODIMP
nsPkcs11::DeleteModule(const nsAString& aModuleName)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (aModuleName.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  NS_ConvertUTF16toUTF8 moduleName(aModuleName);
  
  
#ifndef MOZ_NO_SMART_CARDS
  {
    ScopedSECMODModule module(SECMOD_FindModule(moduleName.get()));
    if (!module) {
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsINSSComponent> nssComponent(
      do_GetService(PSM_COMPONENT_CONTRACTID));
    nssComponent->ShutdownSmartCardThread(module.get());
  }
#endif

  
  int32_t modType;
  SECStatus srv = SECMOD_DeleteModule(moduleName.get(), &modType);
  if (srv != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsPkcs11::AddModule(const nsAString& aModuleName,
                    const nsAString& aLibraryFullPath,
                    int32_t aCryptoMechanismFlags,
                    int32_t aCipherFlags)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (aModuleName.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  NS_ConvertUTF16toUTF8 moduleName(aModuleName);
  nsCString fullPath;
  
  NS_CopyUnicodeToNative(aLibraryFullPath, fullPath);
  uint32_t mechFlags = SECMOD_PubMechFlagstoInternal(aCryptoMechanismFlags);
  uint32_t cipherFlags = SECMOD_PubCipherFlagstoInternal(aCipherFlags);
  SECStatus srv = SECMOD_AddNewModule(moduleName.get(), fullPath.get(),
                                      mechFlags, cipherFlags);
  if (srv != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

#ifndef MOZ_NO_SMART_CARDS
  ScopedSECMODModule module(SECMOD_FindModule(moduleName.get()));
  if (!module) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsINSSComponent> nssComponent(
    do_GetService(PSM_COMPONENT_CONTRACTID));
  nssComponent->LaunchSmartCardThread(module.get());
#endif

  return NS_OK;
}
