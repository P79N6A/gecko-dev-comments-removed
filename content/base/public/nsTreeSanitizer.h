




































#ifndef nsTreeSanitizer_h_
#define nsTreeSanitizer_h_

#include "nsIContent.h"
#include "mozilla/css/StyleRule.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/Element.h"

class NS_STACK_CLASS nsTreeSanitizer {

  public:

    





    nsTreeSanitizer(bool aAllowStyles, bool aAllowComments);

    static void InitializeStatics();
    static void ReleaseStatics();

    






    void Sanitize(nsIContent* aFragment);

  private:

    


    bool mAllowStyles;

    


    bool mAllowComments;

    






    bool MustFlatten(PRInt32 aNamespace, nsIAtom* aLocal);

    







    bool MustPrune(PRInt32 aNamespace,
                     nsIAtom* aLocal,
                     mozilla::dom::Element* aElement);

    






    bool IsURL(nsIAtom*** aURLs, nsIAtom* aLocalName);

    













    void SanitizeAttributes(mozilla::dom::Element* aElement,
                            nsTHashtable<nsISupportsHashKey>* aAllowed,
                            nsIAtom*** aURLs,
                            bool aAllowXLink,
                            bool aAllowStyle,
                            bool aAllowDangerousSrc);

    








    bool SanitizeURL(mozilla::dom::Element* aElement,
                       PRInt32 aNamespace,
                       nsIAtom* aLocalName);

    








    bool SanitizeStyleRule(mozilla::css::StyleRule* aRule,
                             nsAutoString &aRuleText);

    











    bool SanitizeStyleSheet(const nsAString& aOriginal,
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
