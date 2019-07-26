




#ifndef TRANSFRMX_URIUTILS_H
#define TRANSFRMX_URIUTILS_H

#include "txCore.h"

class nsIDocument;
class nsIDOMNode;






class URIUtils {
public:

    


    static void ResetWithSource(nsIDocument *aNewDoc, nsIDOMNode *aSourceNode);

    




    static void resolveHref(const nsAString& href, const nsAString& base,
                            nsAString& dest);
}; 


#endif
