






































#ifndef nsHashPropertyBag_h___
#define nsHashPropertyBag_h___

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsVoidArray.h"

#include "nsIVariant.h"
#include "nsIWritablePropertyBag.h"
#include "nsIWritablePropertyBag2.h"
#include "nsInterfaceHashtable.h"



#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY

class NS_COM nsHashPropertyBag : public nsIWritablePropertyBag
                               , public nsIWritablePropertyBag2
{
public:
    nsHashPropertyBag() { }
    virtual ~nsHashPropertyBag() {}

    nsresult Init();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIPROPERTYBAG

    NS_DECL_NSIPROPERTYBAG2

    NS_DECL_NSIWRITABLEPROPERTYBAG

    NS_DECL_NSIWRITABLEPROPERTYBAG2

protected:
    
    nsInterfaceHashtable<nsStringHashKey, nsIVariant> mPropertyHash;
};

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN



extern "C" NS_COM nsresult
NS_NewHashPropertyBag(nsIWritablePropertyBag* *_retval);

#endif 
