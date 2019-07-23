




































#ifndef nsUcharUtilConstructors_h__
#define nsUcharUtilConstructors_h__

#include "nsUnicharUtilCIID.h"
#include "nsCaseConversionImp2.h"
#include "nsHankakuToZenkakuCID.h"
#include "nsTextTransformFactory.h"
#include "nsICaseConversion.h"
#include "nsEntityConverter.h"
#include "nsSaveAsCharset.h"
#include "nsUUDll.h"
#include "nsIFile.h"
#include "nsUnicodeNormalizer.h"




#define UNICHARUTIL_MAKE_CTOR(_name)                                 \
static NS_IMETHODIMP                                                 \
CreateNew##_name(nsISupports* aOuter, REFNSIID aIID, void **aResult) \
{                                                                    \
    if (!aResult) {                                                  \
        return NS_ERROR_INVALID_POINTER;                             \
    }                                                                \
    if (aOuter) {                                                    \
        *aResult = nsnull;                                           \
        return NS_ERROR_NO_AGGREGATION;                              \
    }                                                                \
    nsISupports* inst;                                               \
    nsresult rv = NS_New##_name(&inst);                              \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
        return rv;                                                   \
    }                                                                \
    rv = inst->QueryInterface(aIID, aResult);                        \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nsnull;                                           \
    }                                                                \
    NS_RELEASE(inst);                /* get rid of extra refcnt */   \
    return rv;                                                       \
}


UNICHARUTIL_MAKE_CTOR(HankakuToZenkaku)

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsCaseConversionImp2,
                                         nsCaseConversionImp2::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsEntityConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSaveAsCharset)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUnicodeNormalizer)

#endif
