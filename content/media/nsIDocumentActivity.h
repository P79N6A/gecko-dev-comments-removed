




#ifndef nsIDocumentActivity_h__
#define nsIDocumentActivity_h__

#include "nsISupports.h"

#define NS_IDOCUMENTACTIVITY_IID \
{ 0x9b9f584e, 0xefa8, 0x11e3, \
  { 0xbb, 0x74, 0x5e, 0xdd, 0x1d, 0x5d, 0x46, 0xb0 } }

class nsIDocumentActivity : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENTACTIVITY_IID)

  virtual void NotifyOwnerDocumentActivityChanged() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentActivity, NS_IDOCUMENTACTIVITY_IID)


#define NS_DECL_NSIDOCUMENTACTIVITY \
  virtual void NotifyOwnerDocumentActivityChanged() MOZ_OVERRIDE;

#endif 
