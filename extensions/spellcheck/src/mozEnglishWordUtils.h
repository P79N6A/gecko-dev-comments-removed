




































#ifndef mozEnglishWordUtils_h__
#define mozEnglishWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsString.h"
#include "nsIUGenCategory.h"

#include "mozITXTToHTMLConv.h" 
#include "nsCycleCollectionParticipant.h"

class mozEnglishWordUtils : public mozISpellI18NUtil
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL
  NS_DECL_CYCLE_COLLECTION_CLASS(mozEnglishWordUtils)

  mozEnglishWordUtils();
  virtual ~mozEnglishWordUtils();
  
  enum myspCapitalization{
    NoCap,InitCap,AllCap,HuhCap
  };  

protected:
  mozEnglishWordUtils::myspCapitalization captype(const nsString &word);
  bool ucIsAlpha(PRUnichar aChar);

  nsString mLanguage;
  nsString mCharset;
  nsCOMPtr<nsIUGenCategory>   mCategories;
  nsCOMPtr<mozITXTToHTMLConv> mURLDetector; 
};

#endif
