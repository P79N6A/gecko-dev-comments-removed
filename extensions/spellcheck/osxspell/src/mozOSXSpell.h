







































#ifndef mozOSXSpell_h__
#define mozOSXSpell_h__

#include "mozISpellCheckingEngine.h"
#include "mozIPersonalDictionary.h"
#include "nsString.h"
#include "nsCOMPtr.h"



#define MOZ_OSXSPELL_CONTRACTID "@mozilla.org/spellchecker/hunspell;1"
#define MOZ_OSXSPELL_CID         \
{ /* BAABBAF4-71C3-47F4-A576-E75469E485E2 */  \
0xBAABBAF4, 0x71C3, 0x47F4,                    \
{ 0xA5, 0x76, 0xE7, 0x54, 0x69, 0xE4, 0x85, 0xE2} }

class mozOSXSpell : public mozISpellCheckingEngine
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLCHECKINGENGINE

  mozOSXSpell();

private:
 
  ~mozOSXSpell();

  
  
  
  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;

  nsString  mLanguage;    
};

#endif
