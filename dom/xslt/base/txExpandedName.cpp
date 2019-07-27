




#include "txExpandedName.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "txStringUtils.h"
#include "txNamespaceMap.h"
#include "txXMLUtils.h"

nsresult
txExpandedName::init(const nsAString& aQName, txNamespaceMap* aResolver,
                     bool aUseDefault)
{
    const nsAFlatString& qName = PromiseFlatString(aQName);
    const char16_t* colon;
    bool valid = XMLUtils::isValidQName(qName, &colon);
    if (!valid) {
        return NS_ERROR_FAILURE;
    }

    if (colon) {
        nsCOMPtr<nsIAtom> prefix = do_GetAtom(Substring(qName.get(), colon));
        int32_t namespaceID = aResolver->lookupNamespace(prefix);
        if (namespaceID == kNameSpaceID_Unknown)
            return NS_ERROR_FAILURE;
        mNamespaceID = namespaceID;

        const char16_t *end;
        qName.EndReading(end);
        mLocalName = do_GetAtom(Substring(colon + 1, end));
    }
    else {
        mNamespaceID = aUseDefault ? aResolver->lookupNamespace(nullptr) :
                                     kNameSpaceID_None;
        mLocalName = do_GetAtom(aQName);
    }
    return NS_OK;
}
