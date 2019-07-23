





































#include "txXMLOutput.h"

const int txXMLOutput::DEFAULT_INDENT = 2;

txOutAttr::txOutAttr(PRInt32 aNsID, nsIAtom* aLocalName,
                         const nsAString& aValue) :
    mName(aNsID, aLocalName),
    mValue(aValue),
    mShorthand(MB_FALSE)
{
}

txXMLOutput::txXMLOutput(txOutputFormat* aFormat, ostream* aOut)
    : mOut(aOut),
      mUseEmptyElementShorthand(MB_TRUE),
      mHaveDocumentElement(MB_FALSE),
      mStartTagOpen(MB_FALSE),
      mAfterEndTag(MB_FALSE),
      mInCDATASection(MB_FALSE),
      mIndentLevel(0)
{
    mOutputFormat.merge(*aFormat);
    mOutputFormat.setFromDefaults();
}

txXMLOutput::~txXMLOutput()
{
}

nsresult
txXMLOutput::attribute(const nsAString& aName,
                       const PRInt32 aNsID,
                       const nsAString& aValue)
{
    if (!mStartTagOpen) {
        
        return NS_OK;
    }

    txListIterator iter(&mAttributes);
    nsCOMPtr<nsIAtom> localName = do_GetAtom(XMLUtils::getLocalPart(aName));
    txExpandedName att(aNsID, localName);

    txOutAttr* setAtt = 0;
    while ((setAtt = (txOutAttr*)iter.next())) {
         if (setAtt->mName == att) {
             setAtt->mValue = aValue;
             break;
         }
    }
    if (!setAtt) {
        setAtt = new txOutAttr(aNsID, localName, aValue);
        NS_ENSURE_TRUE(setAtt, NS_ERROR_OUT_OF_MEMORY);

        nsresult rv = mAttributes.add(setAtt);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    
    return NS_OK;
}

nsresult
txXMLOutput::characters(const nsAString& aData, PRBool aDOE)
{
    closeStartTag(MB_FALSE);

    if (aDOE) {
        printUTF8Chars(aData);

        return NS_OK;
    }

    if (mInCDATASection) {
        PRUint32 length = aData.Length();

        *mOut << CDATA_START;

        if (length <= 3) {
            printUTF8Chars(aData);
        }
        else {
            PRUint32 j = 0;
            nsAString::const_iterator iter;
            aData.BeginReading(iter);
            mBuffer[j++] = *(iter++);
            mBuffer[j++] = *(iter++);
            mBuffer[j++] = *(iter++);

            nsAString::const_iterator end;
            aData.EndReading(end);
            while (iter != end) {
                mBuffer[j++] = *(iter++);
                if (mBuffer[(j - 1) % 4] == ']' &&
                    mBuffer[j % 4] == ']' &&
                    mBuffer[(j + 1) % 4] == '>') {
                    *mOut << CDATA_END;
                    *mOut << CDATA_START;
                }
                j = j % 4;
                printUTF8Char(mBuffer[j]);
            }

            j = ++j % 4;
            printUTF8Char(mBuffer[j]);
            j = ++j % 4;
            printUTF8Char(mBuffer[j]);
            j = ++j % 4;
            printUTF8Char(mBuffer[j]);
        }

        *mOut << CDATA_END;
    }
    else {
        printWithXMLEntities(aData);
    }

    return NS_OK;
}

nsresult
txXMLOutput::comment(const nsAString& aData)
{
    closeStartTag(MB_FALSE);

    if (mOutputFormat.mIndent == eTrue) {
        for (PRUint32 i = 0; i < mIndentLevel; i++)
            *mOut << ' ';
    }
    *mOut << COMMENT_START;
    printUTF8Chars(aData);
    *mOut << COMMENT_END;
    if (mOutputFormat.mIndent == eTrue)
        *mOut << endl;

    return NS_OK;
}

nsresult
txXMLOutput::endDocument(nsresult aResult)
{
    return NS_OK;
}

nsresult
txXMLOutput::endElement(const nsAString& aName,
                             const PRInt32 aNsID)
{
    MBool newLine = (mOutputFormat.mIndent == eTrue) && mAfterEndTag;
    MBool writeEndTag = !(mStartTagOpen && mUseEmptyElementShorthand);
    closeStartTag(mUseEmptyElementShorthand);
    if (newLine)
        *mOut << endl;
    if (mOutputFormat.mIndent == eTrue)
        mIndentLevel -= DEFAULT_INDENT;
    if (writeEndTag) {
        if (newLine) {
            for (PRUint32 i = 0; i < mIndentLevel; i++)
                *mOut << ' ';
        }
        *mOut << L_ANGLE_BRACKET << FORWARD_SLASH;
        printUTF8Chars(aName);
        *mOut << R_ANGLE_BRACKET;
    }
    if (mOutputFormat.mIndent == eTrue)
        *mOut << endl;
    mAfterEndTag = MB_TRUE;
    mInCDATASection = mCDATASections.pop() != 0;

    return NS_OK;
}

nsresult
txXMLOutput::processingInstruction(const nsAString& aTarget,
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
    *mOut << PI_END;
    if (mOutputFormat.mIndent == eTrue)
        *mOut << endl;

    return NS_OK;
}

nsresult
txXMLOutput::startDocument()
{
    if (mOutputFormat.mMethod == eMethodNotSet) {
        
        
    }
    if (mOutputFormat.mMethod == eXMLOutput &&
        mOutputFormat.mOmitXMLDeclaration != eTrue) {
      *mOut << PI_START << XML_DECL << DOUBLE_QUOTE;
      *mOut << XML_VERSION;
      *mOut << DOUBLE_QUOTE << " encoding=\"UTF-8\"";
      if (mOutputFormat.mStandalone != eNotSet) {
        *mOut << " standalone=\"";
        *mOut << (mOutputFormat.mStandalone == eFalse ? "no" : "yes") << "\"";
      }
      *mOut << PI_END << endl;
      
    }

    return NS_OK;
}

nsresult
txXMLOutput::startElement(const nsAString& aName,
                          const PRInt32 aNsID)
{
    if (!mHaveDocumentElement) {
        
        mHaveDocumentElement = MB_TRUE;
    }

    MBool newLine = mStartTagOpen || mAfterEndTag;
    closeStartTag(MB_FALSE);

    if (mOutputFormat.mIndent == eTrue) {
        if (newLine) {
            *mOut << endl;
            for (PRUint32 i = 0; i < mIndentLevel; i++)
                *mOut << ' ';
        }
    }
    *mOut << L_ANGLE_BRACKET;
    printUTF8Chars(aName);
    mStartTagOpen = MB_TRUE;
    if (mOutputFormat.mIndent == eTrue)
        mIndentLevel += DEFAULT_INDENT;

    mCDATASections.push((void*)mInCDATASection);
    mInCDATASection = MB_FALSE;

    nsCOMPtr<nsIAtom> localName = do_GetAtom(aName);
    txExpandedName currentElement(aNsID, localName);
    txListIterator iter(&(mOutputFormat.mCDATASectionElements));
    while (iter.hasNext()) {
        if (currentElement == *(txExpandedName*)iter.next()) {
            mInCDATASection = MB_TRUE;
            break;
        }
    }

    return NS_OK;
}

void txXMLOutput::closeStartTag(MBool aUseEmptyElementShorthand)
{
    mAfterEndTag = aUseEmptyElementShorthand;
    if (mStartTagOpen) {
        txListIterator iter(&mAttributes);
        txOutAttr* att;
        while ((att = (txOutAttr*)iter.next())) {
            *mOut << SPACE;
            const char* attrVal;
            att->mName.mLocalName->GetUTF8String(&attrVal);
            *mOut << attrVal;
            if (!att->mShorthand) {
                *mOut << EQUALS << DOUBLE_QUOTE;
                printWithXMLEntities(att->mValue, MB_TRUE);
                *mOut << DOUBLE_QUOTE;
            }
            delete (txOutAttr*)iter.remove();
        }

        if (aUseEmptyElementShorthand)
            *mOut << FORWARD_SLASH;
        *mOut << R_ANGLE_BRACKET;
        mStartTagOpen = MB_FALSE;
    }
}

void txXMLOutput::printUTF8Char(PRUnichar& ch)
{
    

    
    if (ch < 128) {
        *mOut << (char)ch;
    }
    
    else if (ch < 2048) {
        *mOut << (char) (192+(ch/64));        
        *mOut << (char) (128+(ch%64));        
    }
    
    else {
        *mOut << (char) (224+(ch/4096));      
        *mOut << (char) (128+((ch/64)%64));   
        *mOut << (char) (128+(ch%64));        
    }
}

void txXMLOutput::printUTF8Chars(const nsAString& aData)
{
    *mOut << NS_ConvertUTF16toUTF8(aData).get();
}

void txXMLOutput::printWithXMLEntities(const nsAString& aData,
                                       MBool aAttribute)
{
    nsAString::const_iterator iter, end;
    aData.EndReading(end);

    for (aData.BeginReading(iter); iter != end; ++iter) {
        PRUnichar currChar = *iter;
        switch (currChar) {
            case AMPERSAND:
                *mOut << AMP_ENTITY;
                break;
            case APOSTROPHE:
                if (aAttribute)
                    *mOut << APOS_ENTITY;
                else
                    printUTF8Char(currChar);
                break;
            case GT:
                *mOut << GT_ENTITY;
                break;
            case LT:
                *mOut << LT_ENTITY;
                break;
            case QUOTE:
                if (aAttribute)
                    *mOut << QUOT_ENTITY;
                else
                    printUTF8Char(currChar);
                break;
            default:
                printUTF8Char(currChar);
                break;
        }
    }
    *mOut << flush;
}
