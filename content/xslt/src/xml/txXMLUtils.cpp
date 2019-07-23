










































#include "txXMLUtils.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "txAtoms.h"
#include "txStringUtils.h"
#include "txNamespaceMap.h"
#include "txXPathTreeWalker.h"

nsresult
txExpandedName::init(const nsAString& aQName, txNamespaceMap* aResolver,
                     MBool aUseDefault)
{
    const nsAFlatString& qName = PromiseFlatString(aQName);
    const PRUnichar* colon;
    PRBool valid = XMLUtils::isValidQName(qName, &colon);
    if (!valid) {
        return NS_ERROR_FAILURE;
    }

    if (colon) {
        nsCOMPtr<nsIAtom> prefix = do_GetAtom(Substring(qName.get(), colon));
        PRInt32 namespaceID = aResolver->lookupNamespace(prefix);
        if (namespaceID == kNameSpaceID_Unknown)
            return NS_ERROR_FAILURE;
        mNamespaceID = namespaceID;

        const PRUnichar *end;
        qName.EndReading(end);
        mLocalName = do_GetAtom(Substring(colon + 1, end));
    }
    else {
        mNamespaceID = aUseDefault ? aResolver->lookupNamespace(nsnull) :
                                     kNameSpaceID_None;
        mLocalName = do_GetAtom(aQName);
    }
    return NS_OK;
}

  
 



nsresult
XMLUtils::splitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                         nsIAtom **aLocalName, PRInt32* aNameSpaceID)
{
    






    const PRUnichar *uriEnd = nsnull;
    const PRUnichar *nameEnd = nsnull;
    const PRUnichar *pos;
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

    const PRUnichar *nameStart;
    if (uriEnd) {
        *aNameSpaceID =
            txNamespaceManager::getNamespaceID(nsDependentSubstring(aExpatName,
                                                                    uriEnd));
        if (*aNameSpaceID == kNameSpaceID_Unknown) {
            return NS_ERROR_FAILURE;
        }

        nameStart = (uriEnd + 1);
        if (nameEnd)  {
            const PRUnichar *prefixStart = nameEnd + 1;
            *aPrefix = NS_NewAtom(NS_ConvertUTF16toUTF8(prefixStart,
                                                        pos - prefixStart));
            if (!*aPrefix) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
        else {
            nameEnd = pos;
            *aPrefix = nsnull;
        }
    }
    else {
        *aNameSpaceID = kNameSpaceID_None;
        nameStart = aExpatName;
        nameEnd = pos;
        *aPrefix = nsnull;
    }

    *aLocalName = NS_NewAtom(NS_ConvertUTF16toUTF8(nameStart,
                                                   nameEnd - nameStart));

    return *aLocalName ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
XMLUtils::splitQName(const nsAString& aName, nsIAtom** aPrefix,
                     nsIAtom** aLocalName)
{
    const nsAFlatString& qName = PromiseFlatString(aName);
    const PRUnichar* colon;
    PRBool valid = XMLUtils::isValidQName(qName, &colon);
    if (!valid) {
        return NS_ERROR_FAILURE;
    }

    if (colon) {
        const PRUnichar *end;
        qName.EndReading(end);

        *aPrefix = NS_NewAtom(Substring(qName.get(), colon));
        *aLocalName = NS_NewAtom(Substring(colon + 1, end));
    }
    else {
        *aPrefix = nsnull;
        *aLocalName = NS_NewAtom(aName);
    }

    return NS_OK;
}

const nsDependentSubstring XMLUtils::getLocalPart(const nsAString& src)
{
    
    PRInt32 idx = src.FindChar(':');
    if (idx == kNotFound) {
        return Substring(src, 0, src.Length());
    }

    NS_ASSERTION(idx > 0, "This QName looks invalid.");
    return Substring(src, idx + 1, src.Length() - (idx + 1));
}




PRBool XMLUtils::isWhitespace(const nsAFlatString& aText)
{
    nsAFlatString::const_char_iterator start, end;
    aText.BeginReading(start);
    aText.EndReading(end);
    for ( ; start != end; ++start) {
        if (!isWhitespace(*start)) {
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}




void XMLUtils::normalizePIValue(nsAString& piValue)
{
    nsAutoString origValue(piValue);
    PRUint32 origLength = origValue.Length();
    PRUint32 conversionLoop = 0;
    PRUnichar prevCh = 0;
    piValue.Truncate();

    while (conversionLoop < origLength) {
        PRUnichar ch = origValue.CharAt(conversionLoop);
        switch (ch) {
            case '>':
            {
                if (prevCh == '?') {
                    piValue.Append(PRUnichar(' '));
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


MBool XMLUtils::getXMLSpacePreserve(const txXPathNode& aNode)
{
    nsAutoString value;
    txXPathTreeWalker walker(aNode);
    do {
        if (walker.getAttr(txXMLAtoms::space, kNameSpaceID_XML, value)) {
            if (TX_StringEqualsAtom(value, txXMLAtoms::preserve)) {
                return PR_TRUE;
            }
            if (TX_StringEqualsAtom(value, txXMLAtoms::_default)) {
                return PR_FALSE;
            }
        }
    } while (walker.moveToParent());

    return PR_FALSE;
}
