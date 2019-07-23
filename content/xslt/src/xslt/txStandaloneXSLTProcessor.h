






































#ifndef TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H
#define TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H

#include "txStylesheet.h"
#include "txXSLTProcessor.h"
#include "txErrorObserver.h"

#ifndef __BORLANDC__
#include <iostream.h>
#include <fstream.h>
#endif

class txStreamXMLEventHandler;




























class txStandaloneXSLTProcessor : public txXSLTProcessor
{
public:
    



    









    nsresult transform(nsACString& aXMLPath, ostream& aOut,
                       ErrorObserver& aErr);

    









    nsresult transform(nsACString& aXMLPath, nsACString& aXSLPath,
                       ostream& aOut, ErrorObserver& aErr);

    









    nsresult transform(txXPathNode& aXMLDoc, ostream& aOut, ErrorObserver& aErr);

    








    nsresult transform(txXPathNode& aXMLDoc, txStylesheet* aXSLNode,
                       ostream& aOut, ErrorObserver& aErr);

protected:
    






    static void getHrefFromStylesheetPI(Document& xmlDocument, nsAString& href);

    


    static void parseStylesheetPI(const nsAFlatString& data,
                                  nsAString& type,
                                  nsAString& href);

    






    static txXPathNode* parsePath(const nsACString& aPath, ErrorObserver& aErr);
};

#endif
