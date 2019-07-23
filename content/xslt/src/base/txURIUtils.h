




































#ifndef TRANSFRMX_URIUTILS_H
#define TRANSFRMX_URIUTILS_H

#include "txCore.h"
#ifdef TX_EXE
#include <fstream.h>
#include <iostream.h>
#include "nsString.h"
#else
class nsIDocument;
class nsIDOMNode;

#endif






#ifdef TX_EXE
class txParsedURL
{
public:
    void init(const nsAFlatString& aSpec);
    void resolve(const txParsedURL& aRef, txParsedURL& aDest);
    void getFile(nsString& aResult) const
    {
        aResult = mPath + mName;
    }
    nsString mPath, mName, mRef;
};
#endif

class URIUtils {
public:

#ifdef TX_EXE
    


    static const char HREF_PATH_SEP;

    static istream* getInputStream
        (const nsAString& href, nsAString& errMsg);

    



    static void getDocumentBase(const nsAFlatString& href, nsAString& dest);

#else 

    


    static void ResetWithSource(nsIDocument *aNewDoc, nsIDOMNode *aSourceNode);

#endif 

    




    static void resolveHref(const nsAString& href, const nsAString& base,
                            nsAString& dest);
}; 


#endif
