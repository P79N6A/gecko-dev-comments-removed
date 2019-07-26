











#ifndef nsStyleLinkElement_h___
#define nsStyleLinkElement_h___

#include "nsCOMPtr.h"
#include "nsIDOMLinkStyle.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsCSSStyleSheet.h"
#include "nsTArray.h"
#include "mozilla/CORSMode.h"

#define PREFETCH      0x00000001
#define DNS_PREFETCH  0x00000002
#define STYLESHEET    0x00000004
#define NEXT          0x00000008
#define ALTERNATE     0x00000010

class nsIDocument;
class nsIURI;

class nsStyleLinkElement : public nsIDOMLinkStyle,
                           public nsIStyleSheetLinkingElement
{
public:
  nsStyleLinkElement();
  virtual ~nsStyleLinkElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) = 0;

  
  NS_DECL_NSIDOMLINKSTYLE

  nsCSSStyleSheet* GetSheet() const { return mStyleSheet; }

  
  NS_IMETHOD SetStyleSheet(nsCSSStyleSheet* aStyleSheet);
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet);
  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle);
  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate);
  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates);
  NS_IMETHOD GetCharset(nsAString& aCharset);

  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);
  virtual void SetLineNumber(uint32_t aLineNumber);

  static uint32_t ParseLinkTypes(const nsAString& aTypes);
  
  void UpdateStyleSheetInternal() { UpdateStyleSheetInternal(nullptr); }
protected:
  







  nsresult UpdateStyleSheetInternal(nsIDocument *aOldDocument,
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
  







  nsresult DoUpdateStyleSheet(nsIDocument *aOldDocument,
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

