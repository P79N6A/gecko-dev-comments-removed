








#include "txXMLUtils.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsGkAtoms.h"
#include "txStringUtils.h"
#include "txNamespaceMap.h"
#include "txXPathTreeWalker.h"
#include "nsContentUtils.h"

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

  
 



nsresult
XMLUtils::splitExpatName(const char16_t *aExpatName, nsIAtom **aPrefix,
                         nsIAtom **aLocalName, int32_t* aNameSpaceID)
{
    






    const char16_t *uriEnd = nullptr;
    const char16_t *nameEnd = nullptr;
    const char16_t *pos;
    for (pos = aExpatName; *pos; ++pos) {
        if (*pos == kExpatSeparatorChar) {
            if (uriEnd) {
                nameEnd = pos;
            }
            else {
                uriEnd = pos;
            }
        }
    }

    const char16_t *nameStart;
    if (uriEnd) {
        *aNameSpaceID =
            txNamespaceManager::getNamespaceID(nsDependentSubstring(aExpatName,
                                                                    uriEnd));
        if (*aNameSpaceID == kNameSpaceID_Unknown) {
            return NS_ERROR_FAILURE;
        }

        nameStart = (uriEnd + 1);
        if (nameEnd)  {
            const char16_t *prefixStart = nameEnd + 1;
            *aPrefix = NS_NewAtom(Substring(prefixStart, pos)).take();
            if (!*aPrefix) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
        else {
            nameEnd = pos;
            *aPrefix = nullptr;
        }
    }
    else {
        *aNameSpaceID = kNameSpaceID_None;
        nameStart = aExpatName;
        nameEnd = pos;
        *aPrefix = nullptr;
    }

    *aLocalName = NS_NewAtom(Substring(nameStart, nameEnd)).take();

    return *aLocalName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
XMLUtils::splitQName(const nsAString& aName, nsIAtom** aPrefix,
                     nsIAtom** aLocalName)
{
    const nsAFlatString& qName = PromiseFlatString(aName);
    const char16_t* colon;
    bool valid = XMLUtils::isValidQName(qName, &colon);
    if (!valid) {
        return NS_ERROR_FAILURE;
    }

    if (colon) {
        const char16_t *end;
        qName.EndReading(end);

        *aPrefix = NS_NewAtom(Substring(qName.get(), colon)).take();
        *aLocalName = NS_NewAtom(Substring(colon + 1, end)).take();
    }
    else {
        *aPrefix = nullptr;
        *aLocalName = NS_NewAtom(aName).take();
    }

    return NS_OK;
}




bool XMLUtils::isWhitespace(const nsAFlatString& aText)
{
    nsAFlatString::const_char_iterator start, end;
    aText.BeginReading(start);
    aText.EndReading(end);
    for ( ; start != end; ++start) {
        if (!isWhitespace(*start)) {
            return false;
        }
    }
    return true;
}




void XMLUtils::normalizePIValue(nsAString& piValue)
{
    nsAutoString origValue(piValue);
    uint32_t origLength = origValue.Length();
    uint32_t conversionLoop = 0;
    char16_t prevCh = 0;
    piValue.Truncate();

    while (conversionLoop < origLength) {
        char16_t ch = origValue.CharAt(conversionLoop);
        switch (ch) {
            case '>':
            {
                if (prevCh == '?') {
                    piValue.Append(char16_t(' '));
                }
                break;
            }
            default:
            {
                break;
            }
        }
        piValue.Append(ch);
        prevCh = ch;
        ++conversionLoop;
    }
}


bool XMLUtils::isValidQName(const nsAFlatString& aQName,
                            const char16_t** aColon)
{
  return NS_SUCCEEDED(nsContentUtils::CheckQName(aQName, true, aColon));
}


bool XMLUtils::getXMLSpacePreserve(const txXPathNode& aNode)
{
    nsAutoString value;
    txXPathTreeWalker walker(aNode);
    do {
        if (walker.getAttr(nsGkAtoms::space, kNameSpaceID_XML, value)) {
            if (TX_StringEqualsAtom(value, nsGkAtoms::preserve)) {
                return true;
            }
            if (TX_StringEqualsAtom(value, nsGkAtoms::_default)) {
                return false;
            }
        }
    } while (walker.moveToParent());

    return false;
}
