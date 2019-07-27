





#ifndef mozilla_embedding_PrintDataUtils_h
#define mozilla_embedding_PrintDataUtils_h

#include "mozilla/embedding/PPrinting.h"
#include "nsIWebBrowserPrint.h"








namespace mozilla {
namespace embedding {

class MockWebBrowserPrint MOZ_FINAL : public nsIWebBrowserPrint
{
public:
  MockWebBrowserPrint(PrintData aData);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBBROWSERPRINT

private:
  ~MockWebBrowserPrint();
  PrintData mData;
};

} 
} 

#endif
