





































#ifndef MITRE_XMLPARSER_H
#define MITRE_XMLPARSER_H

#include "txCore.h"

class txXPathNode;










extern "C" nsresult
txParseDocumentFromURI(const nsAString& aHref, const txXPathNode& aLoader,
                       nsAString& aErrMsg, txXPathNode** aResult);

#endif
