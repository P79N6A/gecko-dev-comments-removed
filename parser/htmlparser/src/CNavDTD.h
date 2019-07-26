





#ifndef NS_NAVHTMLDTD__
#define NS_NAVHTMLDTD__

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"


class nsIHTMLContentSink;

#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif

class CNavDTD : public nsIDTD
{
#ifdef _MSC_VER
#pragma warning( default : 4275 )
#endif

public:
    CNavDTD();
    virtual ~CNavDTD();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDTD
};

#endif



