











#ifndef __NSHTMLTOKENIZER
#define __NSHTMLTOKENIZER

#include "nsISupports.h"
#include "nsITokenizer.h"
#include "nsIDTD.h"
#include "prtypes.h"
#include "nsDeque.h"
#include "nsScanner.h"

#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif

class nsHTMLTokenizer : public nsITokenizer {
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOKENIZER
  nsHTMLTokenizer(nsDTDMode aParseMode = eDTDMode_quirks,
                  eParserDocType aDocType = eHTML_Quirks,
                  eParserCommands aCommand = eViewNormal,
                  uint32_t aFlags = 0);
  virtual ~nsHTMLTokenizer();

  static uint32_t GetFlags(const nsIContentSink* aSink);
};

#endif


