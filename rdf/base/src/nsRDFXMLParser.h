






































#ifndef nsRDFParser_h__
#define nsRDFParser_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIRDFXMLParser.h"
#include "nsIRDFDataSource.h"




class nsRDFXMLParser : public nsIRDFXMLParser {
public:
    static NS_IMETHODIMP
    Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRDFXMLPARSER

protected:
    nsRDFXMLParser();
    virtual ~nsRDFXMLParser();
};

#endif 
