




































#ifndef nsTreeSanitizer_h_
#define nsTreeSanitizer_h_

#include "nsIContent.h"
#include "mozilla/css/StyleRule.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/Element.h"

class NS_STACK_CLASS nsTreeSanitizer {

  public:

    





    nsTreeSanitizer(PRBool aAllowStyles, PRBool aAllowComments);

    static void InitializeStatics();
    static void ReleaseStatics();

    






    void Sanitize(nsIContent* aFragment);

  private:

    


    PRBool mAllowStyles;

    


    PRBool mAllowComments;

    






    PRBool MustFlatten(PRInt32 aNamespace, nsIAtom* aLocal);

    







    PRBool MustPrune(PRInt32 aNamespace,
                     nsIAtom* aLocal,
                     mozilla::dom::Element* aElement);

    






    PRBool IsURL(nsIAtom*** aURLs, nsIAtom* aLocalName);

    













    void SanitizeAttributes(mozilla::dom::Element* aElement,
                            nsTHashtable<nsISupportsHashKey>* aAllowed,
                            nsIAtom*** aURLs,
                            PRBool aAllowXLink,
                            PRBool aAllowStyle,
                            PRBool aAllowDangerousSrc);

    








    PRBool SanitizeURL(mozilla::dom::Element* aElement,
                       PRInt32 aNamespace,
                       nsIAtom* aLocalName);

    








    PRBool SanitizeStyleRule(mozilla::css::StyleRule* aRule,
                             nsAutoString &aRuleText);

    











    PRBool SanitizeStyleSheet(const nsAString& aOriginal,
                              nsAString& aSanitized,
                              nsIDocument* aDocument,
                              nsIURI* aBaseURI);

    


    static nsTHashtable<nsISupportsHashKey>* sElementsHTML;

    


    static nsTHashtable<nsISupportsHashKey>* sAttributesHTML;

    


    static nsTHashtable<nsISupportsHashKey>* sElementsSVG;

    


    static nsTHashtable<nsISupportsHashKey>* sAttributesSVG;

    


    static nsTHashtable<nsISupportsHashKey>* sElementsMathML;

    


    static nsTHashtable<nsISupportsHashKey>* sAttributesMathML;

    


    static nsIPrincipal* sNullPrincipal;
};

#endif 
