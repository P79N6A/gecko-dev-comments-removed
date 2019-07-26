







#ifndef nsStyleSheetService_h_
#define nsStyleSheetService_h_

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIMemoryReporter.h"
#include "nsIStyleSheetService.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

class nsICategoryManager;
class nsIMemoryReporter;
class nsISimpleEnumerator;
class nsIStyleSheet;

#define NS_STYLESHEETSERVICE_CID \
{0xfcca6f83, 0x9f7d, 0x44e4, {0xa7, 0x4b, 0xb5, 0x94, 0x33, 0xe6, 0xc8, 0xc3}}

#define NS_STYLESHEETSERVICE_CONTRACTID \
  "@mozilla.org/content/style-sheet-service;1"

class nsStyleSheetService MOZ_FINAL
  : public nsIStyleSheetService
  , public nsIMemoryReporter
{
 public:
  nsStyleSheetService();
  ~nsStyleSheetService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTYLESHEETSERVICE
  NS_DECL_NSIMEMORYREPORTER

  nsresult Init();

  nsCOMArray<nsIStyleSheet>* AgentStyleSheets() { return &mSheets[AGENT_SHEET]; }
  nsCOMArray<nsIStyleSheet>* UserStyleSheets() { return &mSheets[USER_SHEET]; }
  nsCOMArray<nsIStyleSheet>* AuthorStyleSheets() { return &mSheets[AUTHOR_SHEET]; }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  static nsStyleSheetService *GetInstance();
  static nsStyleSheetService *gInstance;

 private:

  void RegisterFromEnumerator(nsICategoryManager  *aManager,
                                          const char          *aCategory,
                                          nsISimpleEnumerator *aEnumerator,
                                          uint32_t             aSheetType);

  int32_t FindSheetByURI(const nsCOMArray<nsIStyleSheet> &sheets,
                                     nsIURI *sheetURI);

  
  
  nsresult LoadAndRegisterSheetInternal(nsIURI *aSheetURI,
                                                    uint32_t aSheetType);

  nsCOMArray<nsIStyleSheet> mSheets[3];
};

#endif
