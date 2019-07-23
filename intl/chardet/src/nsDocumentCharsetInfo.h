




































#ifndef nsDocumentCharsetInfo_h__
#define nsDocumentCharsetInfo_h__

#include "nsIFactory.h"
#include "nsIDocumentCharsetInfo.h"

class nsDocumentCharsetInfo : public nsIDocumentCharsetInfo
{
public:
  nsDocumentCharsetInfo ();
  virtual ~nsDocumentCharsetInfo ();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCUMENTCHARSETINFO

private:
  nsCOMPtr<nsIAtom> mForcedCharset;
  nsCOMPtr<nsIAtom> mParentCharset;
  PRInt32          mParentCharsetSource;
};

#endif 
