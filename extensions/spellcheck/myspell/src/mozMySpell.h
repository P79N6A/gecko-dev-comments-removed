






















































#ifndef mozMySpell_h__
#define mozMySpell_h__

#include "myspell.hxx"
#include "mozISpellCheckingEngine.h"
#include "mozIPersonalDictionary.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsInterfaceHashtable.h"
#include "nsWeakReference.h"

#define MOZ_MYSPELL_CONTRACTID "@mozilla.org/spellchecker/myspell;1"
#define MOZ_MYSPELL_CID         \
{ /* D1EE1205-3F96-4a0f-ABFE-09E8C54C9E9A} */  \
0xD1EE1205, 0x3F96, 0x4a0f,                    \
{ 0xAB, 0xFE, 0x09, 0xE8, 0xC5, 0x4C, 0x9E, 0x9A} }

class mozMySpell : public mozISpellCheckingEngine,
                   public nsIObserver,
                   public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLCHECKINGENGINE
  NS_DECL_NSIOBSERVER

  mozMySpell() : mMySpell(nsnull) { }
  virtual ~mozMySpell();

  nsresult Init();

  void LoadDictionaryList();
  void LoadDictionariesFromDir(nsIFile* aDir);

  
  nsresult ConvertCharset(const PRUnichar* aStr, char ** aDst);

protected:
 
  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;
  nsCOMPtr<nsIUnicodeEncoder>      mEncoder; 
  nsCOMPtr<nsIUnicodeDecoder>      mDecoder; 

  
  nsInterfaceHashtable<nsUnicharPtrHashKey, nsIFile> mDictionaries;
  nsString  mDictionary;
  nsString  mLanguage;

  MySpell  *mMySpell;
};

#endif
