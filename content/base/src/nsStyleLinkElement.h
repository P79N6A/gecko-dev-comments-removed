












































#ifndef nsStyleLinkElement_h___
#define nsStyleLinkElement_h___

#include "nsCOMPtr.h"
#include "nsIDOMLinkStyle.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIStyleSheet.h"
#include "nsIParser.h"
#include "nsIURI.h"

class nsIDocument;
class nsStringArray;

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
  NS_IMETHOD InitStyleLinkElement(nsIParser *aParser, PRBool aDontLoadStyle);
  















  NS_IMETHOD UpdateStyleSheet(nsIDocument *aOldDocument = nsnull,
                              nsICSSLoaderObserver* aObserver = nsnull,
                              PRBool aForceUpdate = PR_FALSE);
  NS_IMETHOD SetEnableUpdates(PRBool aEnableUpdates);
  NS_IMETHOD GetCharset(nsAString& aCharset);

  virtual void OverrideBaseURI(nsIURI* aNewBaseURI);
  virtual void SetLineNumber(PRUint32 aLineNumber);

  static void ParseLinkTypes(const nsAString& aTypes, nsStringArray& aResult);

protected:
  virtual void GetStyleSheetURL(PRBool* aIsInline,
                                nsIURI** aURI) = 0;
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 PRBool* aIsAlternate) = 0;


  nsCOMPtr<nsIStyleSheet> mStyleSheet;
  nsCOMPtr<nsIParser> mParser;
  PRPackedBool mDontLoadStyle;
  PRPackedBool mUpdatesEnabled;
  PRUint32 mLineNumber;
};

#endif 

