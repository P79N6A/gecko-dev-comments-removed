











#ifndef __NSHTMLTOKENIZER
#define __NSHTMLTOKENIZER

#include "mozilla/Attributes.h"
#include "nsISupports.h"
#include "nsITokenizer.h"

#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif

class nsHTMLTokenizer MOZ_FINAL : public nsITokenizer {
  ~nsHTMLTokenizer() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOKENIZER
  nsHTMLTokenizer();
};

#endif


