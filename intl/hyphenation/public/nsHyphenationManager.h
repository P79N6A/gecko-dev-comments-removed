




































#ifndef nsHyphenationManager_h__
#define nsHyphenationManager_h__

#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/Omnijar.h"

class nsHyphenator;
class nsIAtom;
class nsIURI;

class nsHyphenationManager
{
public:
  nsHyphenationManager();

  already_AddRefed<nsHyphenator> GetHyphenator(nsIAtom *aLocale);

  static nsHyphenationManager *Instance();

  static void Shutdown();

private:
  ~nsHyphenationManager();

protected:
  void LoadPatternList();
  void LoadPatternListFromOmnijar(mozilla::Omnijar::Type aType);
  void LoadPatternListFromDir(nsIFile *aDir);
  void LoadAliases();

  nsInterfaceHashtable<nsISupportsHashKey,nsIAtom> mHyphAliases;
  nsInterfaceHashtable<nsISupportsHashKey,nsIURI> mPatternFiles;
  nsRefPtrHashtable<nsISupportsHashKey,nsHyphenator> mHyphenators;

  static nsHyphenationManager *sInstance;
};

#endif 
