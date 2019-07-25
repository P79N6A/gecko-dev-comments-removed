




#ifndef _NSTHEBESFONTENUMERATOR_H_
#define _NSTHEBESFONTENUMERATOR_H_

#include "nsIFontEnumerator.h"
#include "mozilla/Attributes.h"

class nsThebesFontEnumerator MOZ_FINAL : public nsIFontEnumerator
{
public:
    nsThebesFontEnumerator();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIFONTENUMERATOR
};

#endif
