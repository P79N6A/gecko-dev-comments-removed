




#ifndef mozSpellI18NManager_h__
#define mozSpellI18NManager_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NManager.h"
#include "nsCycleCollectionParticipant.h"

#define MOZ_SPELLI18NMANAGER_CONTRACTID "@mozilla.org/spellchecker/i18nmanager;1"
#define MOZ_SPELLI18NMANAGER_CID         \
{ /* {AEB8936F-219C-4D3C-8385-D9382DAA551A} */  \
0xaeb8936f, 0x219c, 0x4d3c, \
  { 0x83, 0x85, 0xd9, 0x38, 0x2d, 0xaa, 0x55, 0x1a } }

class mozSpellI18NManager : public mozISpellI18NManager
{
protected:
  virtual ~mozSpellI18NManager();
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLI18NMANAGER

  mozSpellI18NManager();
};
#endif

