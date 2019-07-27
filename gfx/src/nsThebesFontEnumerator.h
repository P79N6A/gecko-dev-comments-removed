




#ifndef _NSTHEBESFONTENUMERATOR_H_
#define _NSTHEBESFONTENUMERATOR_H_

#include "mozilla/Attributes.h"         
#include "nsIFontEnumerator.h"          
#include "nsISupports.h"                

class nsThebesFontEnumerator final : public nsIFontEnumerator
{
    ~nsThebesFontEnumerator() {}
public:
    nsThebesFontEnumerator();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIFONTENUMERATOR
};

#endif
