





#ifndef mozilla_a11y_xpcAccessibleDocument_h_
#define mozilla_a11y_xpcAccessibleDocument_h_

#include "nsIAccessibleDocument.h"

#include "DocAccessible.h"
#include "nsAccessibilityService.h"
#include "xpcAccessibleApplication.h"
#include "xpcAccessibleHyperText.h"

namespace mozilla {
namespace a11y {




class xpcAccessibleDocument : public xpcAccessibleHyperText,
                              public nsIAccessibleDocument
{
public:
  xpcAccessibleDocument(DocAccessible* aIntl) :
    xpcAccessibleHyperText(aIntl), mCache(kDefaultCacheLength) { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(xpcAccessibleDocument,
                                           xpcAccessibleGeneric)

  
  NS_IMETHOD GetURL(nsAString& aURL) MOZ_FINAL;
  NS_IMETHOD GetTitle(nsAString& aTitle) MOZ_FINAL;
  NS_IMETHOD GetMimeType(nsAString& aType) MOZ_FINAL;
  NS_IMETHOD GetDocType(nsAString& aType) MOZ_FINAL;
  NS_IMETHOD GetDOMDocument(nsIDOMDocument** aDOMDocument) MOZ_FINAL;
  NS_IMETHOD GetWindow(nsIDOMWindow** aDOMWindow) MOZ_FINAL;
  NS_IMETHOD GetParentDocument(nsIAccessibleDocument** aDocument) MOZ_FINAL;
  NS_IMETHOD GetChildDocumentCount(uint32_t* aCount) MOZ_FINAL;
  NS_IMETHOD GetChildDocumentAt(uint32_t aIndex,
                                nsIAccessibleDocument** aDocument) MOZ_FINAL;
  NS_IMETHOD GetVirtualCursor(nsIAccessiblePivot** aVirtualCursor) MOZ_FINAL;

  


  xpcAccessibleGeneric* GetAccessible(Accessible* aAccessible);

  virtual void Shutdown() MOZ_OVERRIDE;

protected:
  virtual ~xpcAccessibleDocument() {}

private:
  DocAccessible* Intl() { return mIntl->AsDoc(); }

  void NotifyOfShutdown(Accessible* aAccessible)
  {
    xpcAccessibleGeneric* xpcAcc = mCache.GetWeak(aAccessible);
    if (xpcAcc)
      xpcAcc->Shutdown();

    mCache.Remove(aAccessible);
  }

  friend class DocManager;
  friend class DocAccessible;

  xpcAccessibleDocument(const xpcAccessibleDocument&) MOZ_DELETE;
  xpcAccessibleDocument& operator =(const xpcAccessibleDocument&) MOZ_DELETE;

  nsRefPtrHashtable<nsPtrHashKey<const Accessible>, xpcAccessibleGeneric> mCache;
};

inline xpcAccessibleGeneric*
ToXPC(Accessible* aAccessible)
{
  if (!aAccessible)
    return nullptr;

  if (aAccessible->IsApplication())
    return XPCApplicationAcc();

  xpcAccessibleDocument* xpcDoc =
    GetAccService()->GetXPCDocument(aAccessible->Document());
  return xpcDoc ? xpcDoc->GetAccessible(aAccessible) : nullptr;
}

inline xpcAccessibleHyperText*
ToXPCText(HyperTextAccessible* aAccessible)
{
  if (!aAccessible)
    return nullptr;

  xpcAccessibleDocument* xpcDoc =
    GetAccService()->GetXPCDocument(aAccessible->Document());
  return static_cast<xpcAccessibleHyperText*>(xpcDoc->GetAccessible(aAccessible));
}

inline xpcAccessibleDocument*
ToXPCDocument(DocAccessible* aAccessible)
{
  return GetAccService()->GetXPCDocument(aAccessible);
}

} 
} 

#endif
