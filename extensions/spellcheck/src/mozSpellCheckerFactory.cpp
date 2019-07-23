




































#include "nsIGenericFactory.h"

#ifdef MOZ_WIDGET_COCOA
#include "mozOSXSpell.h"
#else
#include "mozHunspell.h"
#ifdef MOZ_XUL_APP
#include "mozHunspellDirProvider.h"
#endif
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







#ifdef MOZ_WIDGET_COCOA
NS_GENERIC_FACTORY_CONSTRUCTOR(mozOSXSpell)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozHunspell, Init)
#ifdef MOZ_XUL_APP
NS_GENERIC_FACTORY_CONSTRUCTOR(mozHunspellDirProvider)
#endif
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozSpellChecker, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(mozPersonalDictionary, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(mozSpellI18NManager)







static NS_IMETHODIMP
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






static nsModuleComponentInfo components[] = {
#ifdef MOZ_WIDGET_COCOA
    {
        "OSX Spell check service",
        MOZ_OSXSPELL_CID,
        MOZ_OSXSPELL_CONTRACTID,
        mozOSXSpellConstructor
    },
#else
    {
        "mozHunspell",
        MOZ_HUNSPELL_CID,
        MOZ_HUNSPELL_CONTRACTID,
        mozHunspellConstructor
    },
#ifdef MOZ_XUL_APP
    {
        "mozHunspellDirProvider",
        HUNSPELLDIRPROVIDER_CID,
        mozHunspellDirProvider::kContractID,
        mozHunspellDirProviderConstructor,
        mozHunspellDirProvider::Register,
        mozHunspellDirProvider::Unregister
    },
#endif 
#endif 
  {
      NULL,
      NS_SPELLCHECKER_CID,
      NS_SPELLCHECKER_CONTRACTID,
      mozSpellCheckerConstructor
  },
  {
      NULL,
      MOZ_PERSONALDICTIONARY_CID,
      MOZ_PERSONALDICTIONARY_CONTRACTID,
      mozPersonalDictionaryConstructor
  },
  {
      NULL,
      MOZ_SPELLI18NMANAGER_CID,
      MOZ_SPELLI18NMANAGER_CONTRACTID,
      mozSpellI18NManagerConstructor
  },
  {
      NULL,
      MOZ_INLINESPELLCHECKER_CID,
      MOZ_INLINESPELLCHECKER_CONTRACTID,
      mozInlineSpellCheckerConstructor
  }
};





NS_IMPL_NSGETMODULE(mozSpellCheckerModule, components)
