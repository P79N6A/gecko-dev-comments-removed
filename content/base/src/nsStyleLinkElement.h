











#ifndef nsStyleLinkElement_h___
#define nsStyleLinkElement_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsCSSStyleSheet.h"
#include "nsTArray.h"
#include "mozilla/CORSMode.h"

class nsIDocument;
class nsIURI;

namespace mozilla {
namespace dom {
class ShadowRoot;
} 
} 

class nsStyleLinkElement : public nsIStyleSheetLinkingElement
{
public:
  nsStyleLinkElement();
  virtual ~nsStyleLinkElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) MOZ_OVERRIDE = 0;

  nsCSSStyleSheet* GetSheet() const { return mStyleSheet; }

  
  NS_IMETHOD SetStyleSheet(nsCSSStyleSheet* aStyleSheet) MOZ_OVERRIDE;
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet) MOZ_OVERRIDE;
  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle) MOZ_OVERRIDE;
  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate) MOZ_OVERRIDE;
  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates) MOZ_OVERRIDE;
  NS_IMETHOD GetCharset(nsAString& aCharset) MOZ_OVERRIDE;

  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) MOZ_OVERRIDE;
  virtual void SetLineNumber(uint32_t aLineNumber) MOZ_OVERRIDE;

  enum RelValue {
    ePREFETCH =     0x00000001,
    eDNS_PREFETCH = 0x00000002,
    eSTYLESHEET =   0x00000004,
    eNEXT =         0x00000008,
    eALTERNATE =    0x00000010,
    eHTMLIMPORT =   0x00000020
  };

  
  static uint32_t ParseLinkTypes(const nsAString& aTypes);

  void UpdateStyleSheetInternal()
  {
    UpdateStyleSheetInternal(nullptr, nullptr);
  }
protected:
  







  nsresult UpdateStyleSheetInternal(nsIDocument *aOldDocument,
                                    mozilla::dom::ShadowRoot *aOldShadowRoot,
                                    bool aForceUpdate = false);

  void UpdateStyleSheetScopedness(bool aIsNowScoped);

  virtual already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline) = 0;
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 bool* aIsScoped,
                                 bool* aIsAlternate) = 0;

  virtual mozilla::CORSMode GetCORSMode() const
  {
    
    return mozilla::CORS_NONE;
  }

  
  void Unlink();
  void Traverse(nsCycleCollectionTraversalCallback &cb);

private:
  











  nsresult DoUpdateStyleSheet(nsIDocument* aOldDocument,
                              mozilla::dom::ShadowRoot* aOldShadowRoot,
                              nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate,
                              bool aForceUpdate);

  nsRefPtr<nsCSSStyleSheet> mStyleSheet;
protected:
  bool mDontLoadStyle;
  bool mUpdatesEnabled;
  uint32_t mLineNumber;
};

#endif 

