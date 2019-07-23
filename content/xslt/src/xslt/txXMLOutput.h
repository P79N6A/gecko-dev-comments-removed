





































#ifndef TRANSFRMX_XML_OUTPUT_H
#define TRANSFRMX_XML_OUTPUT_H

#include "txXMLEventHandler.h"
#include "txList.h"
#include "txStack.h"
#include "txOutputFormat.h"
#include "txXMLUtils.h"

#define DASH            '-'
#define TX_CR           '\r'
#define TX_LF           '\n'

#define AMPERSAND       '&'
#define APOSTROPHE      '\''
#define GT              '>'
#define LT              '<'
#define QUOTE           '"'

#define AMP_ENTITY      "&amp;"
#define APOS_ENTITY     "&apos;"
#define GT_ENTITY       "&gt;"
#define LT_ENTITY       "&lt;"
#define QUOT_ENTITY     "&quot;"
#define HEX_ENTITY      "&#"

#define CDATA_END       "]]>"
#define CDATA_START     "<![CDATA["
#define COMMENT_START   "<!--"
#define COMMENT_END     "-->"
#define DOCTYPE_START   "<!DOCTYPE "
#define DOCTYPE_END     ">"
#define DOUBLE_QUOTE    "\""
#define EQUALS          "="
#define FORWARD_SLASH   "/"
#define L_ANGLE_BRACKET "<"
#define PI_START        "<?"
#define PI_END          "?>"
#define PUBLIC          "PUBLIC"
#define R_ANGLE_BRACKET ">"
#define SEMICOLON       ";"
#define SPACE           " "
#define SYSTEM          "SYSTEM"
#define XML_DECL        "xml version="
#define XML_VERSION     "1.0"

class txOutAttr {
public:
    txOutAttr(PRInt32 aNsID, nsIAtom* aLocalName, const nsAString& aValue);
    txExpandedName mName;
    nsString mValue;
    MBool mShorthand;
};

class txXMLOutput : public txAOutputXMLEventHandler
{
public:
    txXMLOutput(txOutputFormat* aFormat, ostream* aOut);
    virtual ~txXMLOutput();

    static const int DEFAULT_INDENT;

    TX_DECL_TXAXMLEVENTHANDLER
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

protected:
    virtual void closeStartTag(MBool aUseEmptyElementShorthand);
    void printUTF8Char(PRUnichar& ch);
    void printUTF8Chars(const nsAString& aData);
    void printWithXMLEntities(const nsAString& aData,
                              MBool aAttribute = MB_FALSE);

    ostream* mOut;
    txOutputFormat mOutputFormat;
    MBool mUseEmptyElementShorthand;
    MBool mHaveDocumentElement;
    MBool mStartTagOpen;
    MBool mAfterEndTag;
    MBool mInCDATASection;
    PRUint32 mIndentLevel;
    txList mAttributes;
    txStack mCDATASections;

private:
    PRUnichar mBuffer[4];
};

#endif
