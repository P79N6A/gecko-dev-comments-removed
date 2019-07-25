












































#ifndef nsStyleLinkElement_h___
#define nsStyleLinkElement_h___

#include "nsCOMPtr.h"
#include "nsIDOMLinkStyle.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIStyleSheet.h"
#include "nsIURI.h"
#include "nsTArray.h"

#define PREFETCH      0x00000001
#define DNS_PREFETCH  0x00000002
#define STYLESHEET    0x00000004
#define NEXT          0x00000008
#define ALTERNATE     0x00000010

class nsIDocument;

class nsStyleLinkElement : public nsIDOMLinkStyle,
                           public nsIStyleSheetLinkingElement
{
public:
  nsStyleLinkElement();
  virtual ~nsStyleLinkElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) = 0;

  
  NS_DECL_NSIDOMLINKSTYLE

  
  NS_IMETHOD SetStyleSheet(nsIStyleSheet* aStyleSheet);
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet);
  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle);
  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate);
  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates);
  NS_IMETHOD GetCharset(nsAString& aCharset);

  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);
  virtual void SetLineNumber(PRUint32 aLineNumber);

  static PRUint32 ParseLinkTypes(const nsAString& aTypes);
  
  void UpdateStyleSheetInternal() { UpdateStyleSheetInternal(nsnull); }
protected:
  







  nsresult UpdateStyleSheetInternal(nsIDocument *aOldDocument,
                                    bool aForceUpdate = false);

  virtual already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline) = 0;
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 bool* aIsAlternate) = 0;

  nsIStyleSheet* GetStyleSheet() { return mStyleSheet; }

private:
  







  nsresult DoUpdateStyleSheet(nsIDocument *aOldDocument,
                              nsICSSLoaderObserver* aObserver,
                              bool* aWillNotify,
                              bool* aIsAlternate,
                              bool aForceUpdate);

  nsCOMPtr<nsIStyleSheet> mStyleSheet;
protected:
  bool mDontLoadStyle;
  bool mUpdatesEnabled;
  PRUint32 mLineNumber;
};

#endif 

