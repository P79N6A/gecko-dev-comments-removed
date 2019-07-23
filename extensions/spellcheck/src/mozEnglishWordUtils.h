




































#ifndef mozEnglishWordUtils_h__
#define mozEnglishWordUtils_h__

#include "nsCOMPtr.h"
#include "mozISpellI18NUtil.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsString.h"
#include "nsICaseConversion.h"

#include "mozITXTToHTMLConv.h" 

class mozEnglishWordUtils : public mozISpellI18NUtil
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISPELLI18NUTIL

  mozEnglishWordUtils();
  virtual ~mozEnglishWordUtils();
  
  enum myspCapitalization{
    NoCap,InitCap,AllCap,HuhCap
  };  

protected:
  mozEnglishWordUtils::myspCapitalization captype(const nsString &word);

  nsString mLanguage;
  nsString mCharset;
  nsCOMPtr<nsICaseConversion> mCaseConv;
  nsCOMPtr<mozITXTToHTMLConv> mURLDetector; 
};

#endif
