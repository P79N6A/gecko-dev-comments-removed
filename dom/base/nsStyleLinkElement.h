











#ifndef nsStyleLinkElement_h___
#define nsStyleLinkElement_h___

#include "mozilla/Attributes.h"
#include "mozilla/CORSMode.h"
#include "mozilla/CSSStyleSheet.h"
#include "nsCOMPtr.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsTArray.h"

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

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override = 0;

  mozilla::CSSStyleSheet* GetSheet() const { return mStyleSheet; }

  
  NS_IMETHOD SetStyleSheet(mozilla::CSSStyleSheet* aStyleSheet) override;
  NS_IMETHOD_(mozilla::CSSStyleSheet*) GetStyleSheet() override;
  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle) override;
  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate,
                              bool aForceReload) override;
  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates) override;
  NS_IMETHOD GetCharset(nsAString& aCharset) override;

  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) override;
  virtual void SetLineNumber(uint32_t aLineNumber) override;

  enum RelValue {
    ePREFETCH =     0x00000001,
    eDNS_PREFETCH = 0x00000002,
    eSTYLESHEET =   0x00000004,
    eNEXT =         0x00000008,
    eALTERNATE =    0x00000010,
    eHTMLIMPORT =   0x00000020,
    ePRECONNECT =   0x00000040
  };

  
  
  
  static uint32_t ParseLinkTypes(const nsAString& aTypes,
                                 nsIPrincipal* aPrincipal);

  static bool IsImportEnabled();
  
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

  nsRefPtr<mozilla::CSSStyleSheet> mStyleSheet;
protected:
  bool mDontLoadStyle;
  bool mUpdatesEnabled;
  uint32_t mLineNumber;
};

#endif 

