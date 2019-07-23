





































#include "txHTMLOutput.h"
#include "nsCOMArray.h"
#include "nsStaticNameTable.h"
#include "txAtoms.h"
#include "txOutputFormat.h"
#include "txStringUtils.h"
#include "txXMLUtils.h"

#define EMPTY_ELEMENTS_COUNT 13
const char* const kHTMLEmptyTags[] =
{
    "area",
    "base",
    "basefont",
    "br",
    "col",
    "frame",
    "hr",
    "img",
    "input",
    "isindex",
    "link",
    "meta",
    "param"
};

#define SHORTHAND_ATTR_COUNT 12
const char* const kHTMLEmptyAttributes[] =
{
    "checked",
    "compact",
    "declare",
    "defer",
    "disabled",
    "ismap",
    "multiple",
    "noresize",
    "noshade",
    "nowrap",
    "readonly",
    "selected"
};

struct txEmptyAttributesMaps
{
    typedef nsCOMArray<nsIAtom> EmptyAttrBag;
    EmptyAttrBag mMaps[SHORTHAND_ATTR_COUNT];
};

static PRInt32 gTableRefCount;
static nsStaticCaseInsensitiveNameTable* gHTMLEmptyTagsTable;
static nsStaticCaseInsensitiveNameTable* gHTMLEmptyAttributesTable;
static txEmptyAttributesMaps* gHTMLEmptyAttributesMaps;


nsresult
txHTMLOutput::init()
{
    if (0 == gTableRefCount++) {
        NS_ASSERTION(!gHTMLEmptyTagsTable, "pre existing array!");
        gHTMLEmptyTagsTable = new nsStaticCaseInsensitiveNameTable();
        if (!gHTMLEmptyTagsTable) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        gHTMLEmptyTagsTable->Init(kHTMLEmptyTags, EMPTY_ELEMENTS_COUNT);

        NS_ASSERTION(!gHTMLEmptyAttributesTable, "pre existing array!");
        gHTMLEmptyAttributesTable = new nsStaticCaseInsensitiveNameTable();
        if (!gHTMLEmptyAttributesTable) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        gHTMLEmptyAttributesTable->Init(kHTMLEmptyAttributes,
                                        SHORTHAND_ATTR_COUNT);

        NS_ASSERTION(!gHTMLEmptyAttributesMaps, "pre existing map!");
        gHTMLEmptyAttributesMaps = new txEmptyAttributesMaps();
        if (!gHTMLEmptyAttributesMaps) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        
        gHTMLEmptyAttributesMaps->mMaps[0].AppendObject(txHTMLAtoms::input);

        
        gHTMLEmptyAttributesMaps->mMaps[1].AppendObject(txHTMLAtoms::dir);
        gHTMLEmptyAttributesMaps->mMaps[1].AppendObject(txHTMLAtoms::dl);
        gHTMLEmptyAttributesMaps->mMaps[1].AppendObject(txHTMLAtoms::menu);
        gHTMLEmptyAttributesMaps->mMaps[1].AppendObject(txHTMLAtoms::ol);
        gHTMLEmptyAttributesMaps->mMaps[1].AppendObject(txHTMLAtoms::ul);

        
        gHTMLEmptyAttributesMaps->mMaps[2].AppendObject(txHTMLAtoms::object);

        
        gHTMLEmptyAttributesMaps->mMaps[3].AppendObject(txHTMLAtoms::script);

        
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::button);
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::input);
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::optgroup);
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::option);
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::select);
        gHTMLEmptyAttributesMaps->mMaps[4].AppendObject(txHTMLAtoms::textarea);

        
        gHTMLEmptyAttributesMaps->mMaps[5].AppendObject(txHTMLAtoms::img);
        gHTMLEmptyAttributesMaps->mMaps[5].AppendObject(txHTMLAtoms::input);

        
        gHTMLEmptyAttributesMaps->mMaps[6].AppendObject(txHTMLAtoms::select);

        
        gHTMLEmptyAttributesMaps->mMaps[7].AppendObject(txHTMLAtoms::frame);

        
        gHTMLEmptyAttributesMaps->mMaps[8].AppendObject(txHTMLAtoms::hr);

        
        gHTMLEmptyAttributesMaps->mMaps[9].AppendObject(txHTMLAtoms::td);
        gHTMLEmptyAttributesMaps->mMaps[9].AppendObject(txHTMLAtoms::th);

        
        gHTMLEmptyAttributesMaps->mMaps[10].AppendObject(txHTMLAtoms::input);
        gHTMLEmptyAttributesMaps->mMaps[10].AppendObject(txHTMLAtoms::textarea);

        
        gHTMLEmptyAttributesMaps->mMaps[11].AppendObject(txHTMLAtoms::option);
    }

    return NS_OK;
}


void
txHTMLOutput::shutdown()
{
    if (0 == --gTableRefCount) {
        if (gHTMLEmptyTagsTable) {
            delete gHTMLEmptyTagsTable;
            gHTMLEmptyTagsTable = nsnull;
        }
         if (gHTMLEmptyAttributesTable) {
            delete gHTMLEmptyAttributesTable;
            gHTMLEmptyAttributesTable = nsnull;
        }
        if (gHTMLEmptyAttributesMaps) {
            delete gHTMLEmptyAttributesMaps;
            gHTMLEmptyAttributesMaps = nsnull;
        }
   }
}

txHTMLOutput::txHTMLOutput(txOutputFormat* aFormat, ostream* aOut)
    : txXMLOutput(aFormat, aOut)
{
    mUseEmptyElementShorthand = PR_FALSE;
}

txHTMLOutput::~txHTMLOutput()
{
}

nsresult
txHTMLOutput::attribute(const nsAString& aName, const PRInt32 aNsID,
                        const nsAString& aValue)
{
    if (!mStartTagOpen)
        
        return;

    MBool shortHand = MB_FALSE;
    if (aNsID == kNameSpaceID_None) {
        const nsAString& localPart = XMLUtils::getLocalPart(aName);
        shortHand = isShorthandAttribute(localPart);
        if (shortHand &&
            localPart.Equals(aValue, txCaseInsensitiveStringComparator())) {
            txListIterator iter(&mAttributes);
            txOutAttr* setAtt = 0;
            nsCOMPtr<nsIAtom> localName = do_GetAtom(localPart);
            txExpandedName att(aNsID, localName);
            while ((setAtt = (txOutAttr*)iter.next())) {
                 if (setAtt->mName == att) {
                     setAtt->mShorthand = MB_TRUE;
                     break;
                 }
            }
            if (!setAtt) {
                setAtt = new txOutAttr(aNsID, localName, EmptyString());
                setAtt->mShorthand = MB_TRUE;
                mAttributes.add(setAtt);
            }
        }
    }
    if (!shortHand)
        txXMLOutput::attribute(aName, aNsID, aValue);

    return NS_OK;
}

nsresult
txHTMLOutput::characters(const nsAString& aData, PRBool aDOE)
{
    if (aDOE) {
        closeStartTag(MB_FALSE);
        printUTF8Chars(aData);

        return NS_OK;
    }

    
    if (!mCurrentElements.isEmpty()) {
        txExpandedName* currentElement = (txExpandedName*)mCurrentElements.peek();
        if (currentElement->mNamespaceID == kNameSpaceID_None &&
            (currentElement->mLocalName == txHTMLAtoms::script ||
             currentElement->mLocalName == txHTMLAtoms::style)) {
            closeStartTag(MB_FALSE);
            printUTF8Chars(aData);

            return NS_OK;
        }
    }
    return txXMLOutput::characters(aData, aDOE);
}

nsresult
txHTMLOutput::endElement(const nsAString& aName, const PRInt32 aNsID)
{
    nsresult rv = NS_OK;
    const nsAString& localPart = XMLUtils::getLocalPart(aName);
    if ((aNsID == kNameSpaceID_None) && isShorthandElement(localPart) &&
        mStartTagOpen) {
        MBool newLine = (mOutputFormat.mIndent == eTrue) &&
                        mAfterEndTag;
        closeStartTag(MB_FALSE);
        if (newLine)
            *mOut << endl;
        if (mOutputFormat.mIndent == eTrue)
            mIndentLevel -= DEFAULT_INDENT;
        mAfterEndTag = MB_TRUE;
    }
    else {
        rv = txXMLOutput::endElement(aName, aNsID);
    }
    delete (txExpandedName*)mCurrentElements.pop();

    return rv;
}

nsresult
txHTMLOutput::processingInstruction(const nsAString& aTarget,
                                    const nsAString& aData)
{
    closeStartTag(MB_FALSE);
    if (mOutputFormat.mIndent == eTrue) {
        for (PRUint32 i = 0; i < mIndentLevel; i++)
            *mOut << ' ';
    }
    *mOut << PI_START;
    printUTF8Chars(aTarget);
    *mOut << SPACE;
    printUTF8Chars(aData);
    *mOut << R_ANGLE_BRACKET;
    if (mOutputFormat.mIndent == eTrue)
        *mOut << endl;

    return NS_OK;
}

nsresult
txHTMLOutput::startDocument()
{
    
    *mOut << DOCTYPE_START << "html " << PUBLIC;
    *mOut << " \"-//W3C//DTD HTML 4.0 Transitional//EN\"";
    *mOut << " \"http://www.w3.org/TR/REC-html40/loose.dtd\"";
    *mOut << DOCTYPE_END << endl;

    return NS_OK;
}

nsresult
txHTMLOutput::startElement(const nsAString& aName, const PRInt32 aNsID)
{
    nsresult rv = txXMLOutput::startElement(aName, aNsID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAtom> localAtom;
    if (aNsID == kNameSpaceID_None) {
        nsAutoString localName;
        TX_ToLowerCase(aName, localName);
        localAtom = do_GetAtom(localName);
    }
    else {
        localAtom = do_GetAtom(aName);
    }
    NS_ASSERTION(localAtom, "Can't get atom");
    txExpandedName* currentElement = new txExpandedName(aNsID, localAtom);
    NS_ASSERTION(currentElement, "Can't create currentElement");
    if (currentElement)
        mCurrentElements.push(currentElement);

    return NS_OK;
}

void txHTMLOutput::closeStartTag(MBool aUseEmptyElementShorthand)
{
    txExpandedName* currentElement = mCurrentElements.isEmpty() ?
        nsnull : (txExpandedName*)mCurrentElements.peek();
    if (mStartTagOpen && currentElement &&
        (currentElement->mNamespaceID == kNameSpaceID_None) &&
        (currentElement->mLocalName == txHTMLAtoms::head)) {
        txXMLOutput::closeStartTag(MB_FALSE);
        if (mOutputFormat.mIndent == eTrue) {
            *mOut << endl;
            for (PRUint32 i = 0; i < mIndentLevel; i++)
                *mOut << ' ';
        }
        *mOut << LT << "meta http-equiv=" << QUOTE << "Content-Type" << QUOTE;
        *mOut << " content=" << QUOTE;
        printUTF8Chars(mOutputFormat.mMediaType);
        *mOut << "; charset=";
        printUTF8Chars(mOutputFormat.mEncoding);
        *mOut << QUOTE << GT;
    }
    else {
        txXMLOutput::closeStartTag(aUseEmptyElementShorthand);
    }
}

MBool txHTMLOutput::isShorthandElement(const nsAString& aLocalName)
{
    return (gHTMLEmptyTagsTable->Lookup(aLocalName) !=
            nsStaticCaseInsensitiveNameTable::NOT_FOUND);
}

MBool txHTMLOutput::isShorthandAttribute(const nsAString& aLocalName)
{
    PRInt32 index = gHTMLEmptyTagsTable->Lookup(aLocalName);
    if (index == nsStaticCaseInsensitiveNameTable::NOT_FOUND) {
        return PR_FALSE;
    }

    txExpandedName* currentElement = (txExpandedName*)mCurrentElements.peek();
    return (gHTMLEmptyAttributesMaps->mMaps[index].IndexOf(currentElement->mLocalName) > -1);
}
