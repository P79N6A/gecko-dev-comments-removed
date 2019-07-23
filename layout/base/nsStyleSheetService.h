







































#ifndef nsStyleSheetService_h_
#define nsStyleSheetService_h_

#include "nsIStyleSheetService.h"
#include "nsCOMArray.h"
#include "nsIStyleSheet.h"

class nsISimpleEnumerator;
class nsICategoryManager;

#define NS_STYLESHEETSERVICE_CID \
{0xfcca6f83, 0x9f7d, 0x44e4, {0xa7, 0x4b, 0xb5, 0x94, 0x33, 0xe6, 0xc8, 0xc3}}

#define NS_STYLESHEETSERVICE_CONTRACTID \
  "@mozilla.org/content/style-sheet-service;1"

class nsStyleSheetService : public nsIStyleSheetService
{
 public:
  nsStyleSheetService() NS_HIDDEN;
  ~nsStyleSheetService() NS_HIDDEN;

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTYLESHEETSERVICE

  NS_HIDDEN_(nsresult) Init();

  nsCOMArray<nsIStyleSheet>* AgentStyleSheets() { return &mSheets[AGENT_SHEET]; }
  nsCOMArray<nsIStyleSheet>* UserStyleSheets() { return &mSheets[USER_SHEET]; }

  static nsStyleSheetService *gInstance;

 private:

  NS_HIDDEN_(void) RegisterFromEnumerator(nsICategoryManager  *aManager,
                                          const char          *aCategory,
                                          nsISimpleEnumerator *aEnumerator,
                                          PRUint32             aSheetType);

  NS_HIDDEN_(PRInt32) FindSheetByURI(const nsCOMArray<nsIStyleSheet> &sheets,
                                     nsIURI *sheetURI);

  
  
  NS_HIDDEN_(nsresult) LoadAndRegisterSheetInternal(nsIURI *aSheetURI,
                                                    PRUint32 aSheetType);
  
  nsCOMArray<nsIStyleSheet> mSheets[2];
};

#endif
