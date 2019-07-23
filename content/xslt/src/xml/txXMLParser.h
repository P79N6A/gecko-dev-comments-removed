





































#ifndef MITRE_XMLPARSER_H
#define MITRE_XMLPARSER_H

#include "txCore.h"

#ifdef TX_EXE
#include <iostream.h>
#endif

class txXPathNode;










extern "C" nsresult
txParseDocumentFromURI(const nsAString& aHref, const txXPathNode& aLoader,
                       nsAString& aErrMsg, txXPathNode** aResult);

#ifdef TX_EXE



extern "C" nsresult
txParseFromStream(istream& aInputStream, const nsAString& aUri,
                  nsAString& aErrorString, txXPathNode** aResult);
#endif
#endif
