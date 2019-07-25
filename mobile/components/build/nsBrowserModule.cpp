






































#include "mozilla/ModuleUtils.h"

#ifndef ANDROID
#include "nsPhoneSupport.h"
#endif

#include "nsSSLCertErrorDialog.h"

#ifndef ANDROID
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPhoneSupport)
NS_DEFINE_NAMED_CID(nsPhoneSupport_CID);
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSSLCertErrorDialog)
NS_DEFINE_NAMED_CID(nsSSLCertErrorDialog_CID);

static const mozilla::Module::CIDEntry kBrowserCIDs[] = {
#ifndef ANDROID
  { &knsPhoneSupport_CID, false, NULL, nsPhoneSupportConstructor },
#endif
  { &knsSSLCertErrorDialog_CID, false, NULL, nsSSLCertErrorDialogConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserContracts[] = {
#ifndef ANDROID
  { nsPhoneSupport_ContractID, &knsPhoneSupport_CID },
#endif
  { nsSSLCertErrorDialog_ContractID, &knsSSLCertErrorDialog_CID },
  { NULL }
};

static const mozilla::Module kBrowserModule = {
  mozilla::Module::kVersion,
  kBrowserCIDs,
  kBrowserContracts
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kBrowserModule;
