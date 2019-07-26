




#ifndef mozEnglishWordUtils_h__
#define mozEnglishWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsString.h"

#include "mozITXTToHTMLConv.h" 
#include "nsCycleCollectionParticipant.h"

class mozEnglishWordUtils : public mozISpellI18NUtil
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL
  NS_DECL_CYCLE_COLLECTION_CLASS(mozEnglishWordUtils)

  mozEnglishWordUtils();
  
  enum myspCapitalization{
    NoCap,InitCap,AllCap,HuhCap
  };  

protected:
  virtual ~mozEnglishWordUtils();

  mozEnglishWordUtils::myspCapitalization captype(const nsString &word);
  bool ucIsAlpha(char16_t aChar);

  nsString mLanguage;
  nsString mCharset;
  nsCOMPtr<mozITXTToHTMLConv> mURLDetector; 
};

#endif
