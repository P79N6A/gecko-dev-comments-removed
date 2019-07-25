




































#include "mozilla/ModuleUtils.h"

#ifdef MOZ_MACBROWSER
#include "mozOSXSpell.h"
#else
#include "mozHunspell.h"
#include "mozHunspellDirProvider.h"
#endif

#include "mozSpellChecker.h"
#include "mozInlineSpellChecker.h"
#include "nsTextServicesCID.h"
#include "mozPersonalDictionary.h"
#include "mozSpellI18NManager.h"

#define NS_SPELLCHECKER_CID         \
{ /* 8227f019-afc7-461e-b030-9f185d7a0e29 */    \
0x8227F019, 0xAFC7, 0x461e,                     \
{ 0xB0, 0x30, 0x9F, 0x18, 0x5D, 0x7A, 0x0E, 0x29} }

#define MOZ_INLINESPELLCHECKER_CID         \
{ /* 9FE5D975-09BD-44aa-A01A-66402EA28657 */    \
0x9fe5d975, 0x9bd, 0x44aa,                      \
{ 0xa0, 0x1a, 0x66, 0x40, 0x2e, 0xa2, 0x86, 0x57} }

#ifdef MOZ_MACBROWSER
NS_GENERIC_FACTORY_CONSTRUCTOR(mozOSXSpell)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozHunspell, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(mozHunspellDirProvider)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozSpellChecker, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozPersonalDictionary, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(mozSpellI18NManager)







static nsresult
mozInlineSpellCheckerConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult)
{
  if (! mozInlineSpellChecker::CanEnableInlineSpellChecking())
    return NS_ERROR_FAILURE;

  nsresult rv;

  mozInlineSpellChecker* inst;

  *aResult = NULL;
  if (NULL != aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }

  NS_NEWXPCOM(inst, mozInlineSpellChecker);
  if (NULL == inst) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
  }
  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

#ifdef MOZ_MACBROWSER
NS_DEFINE_NAMED_CID(MOZ_OSXSPELL_CID);
#else
NS_DEFINE_NAMED_CID(MOZ_HUNSPELL_CID);
NS_DEFINE_NAMED_CID(HUNSPELLDIRPROVIDER_CID);
#endif 
NS_DEFINE_NAMED_CID(NS_SPELLCHECKER_CID);
NS_DEFINE_NAMED_CID(MOZ_PERSONALDICTIONARY_CID);
NS_DEFINE_NAMED_CID(MOZ_SPELLI18NMANAGER_CID);
NS_DEFINE_NAMED_CID(MOZ_INLINESPELLCHECKER_CID);

static const mozilla::Module::CIDEntry kSpellcheckCIDs[] = {
#ifdef MOZ_MACBROWSER
    { &kMOZ_OSXSPELL_CID, false, NULL, mozOSXSpellConstructor },
#else
    { &kMOZ_HUNSPELL_CID, false, NULL, mozHunspellConstructor },
    { &kHUNSPELLDIRPROVIDER_CID, false, NULL, mozHunspellDirProviderConstructor },
#endif 
    { &kNS_SPELLCHECKER_CID, false, NULL, mozSpellCheckerConstructor },
    { &kMOZ_PERSONALDICTIONARY_CID, false, NULL, mozPersonalDictionaryConstructor },
    { &kMOZ_SPELLI18NMANAGER_CID, false, NULL, mozSpellI18NManagerConstructor },
    { &kMOZ_INLINESPELLCHECKER_CID, false, NULL, mozInlineSpellCheckerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kSpellcheckContracts[] = {
#ifdef MOZ_MACBROWSER
    { MOZ_OSXSPELL_CONTRACTID, &kMOZ_OSXSPELL_CID },
#else
    { MOZ_HUNSPELL_CONTRACTID, &kMOZ_HUNSPELL_CID },
    { mozHunspellDirProvider::kContractID, &kHUNSPELLDIRPROVIDER_CID },
#endif 
    { NS_SPELLCHECKER_CONTRACTID, &kNS_SPELLCHECKER_CID },
    { MOZ_PERSONALDICTIONARY_CONTRACTID, &kMOZ_PERSONALDICTIONARY_CID },
    { MOZ_SPELLI18NMANAGER_CONTRACTID, &kMOZ_SPELLI18NMANAGER_CID },
    { MOZ_INLINESPELLCHECKER_CONTRACTID, &kMOZ_INLINESPELLCHECKER_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kSpellcheckCategories[] = {
#ifndef MOZ_MACBROWSER
    { XPCOM_DIRECTORY_PROVIDER_CATEGORY, "spellcheck-directory-provider", mozHunspellDirProvider::kContractID },
#endif
    { NULL }
};

const mozilla::Module kSpellcheckModule = {
    mozilla::Module::kVersion,
    kSpellcheckCIDs,
    kSpellcheckContracts,
    kSpellcheckCategories
};

NSMODULE_DEFN(mozSpellCheckerModule) = &kSpellcheckModule;
