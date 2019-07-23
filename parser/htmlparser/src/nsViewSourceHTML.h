











































#ifndef __NS_VIEWSOURCE_HTML_
#define __NS_VIEWSOURCE_HTML_

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsHTMLTokens.h"
#include "nsIHTMLContentSink.h"
#include "nsDTDUtils.h"
#include "nsParserNode.h"

class nsIParserNode;
class nsParser;
class nsITokenizer;
class nsCParserNode;

class CViewSourceHTML: public nsIDTD
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDTD
    
    CViewSourceHTML();
    virtual ~CViewSourceHTML();

    






    virtual void SetVerification(PRBool aEnable);

private:
    nsresult WriteTag(PRInt32 tagType,
                      const nsSubstring &aText,
                      PRInt32 attrCount,
                      PRBool aTagInError);
    
    nsresult WriteAttributes(PRInt32 attrCount, PRBool aOwnerInError);
    void StartNewPreBlock(void);
    
    void AddAttrToNode(nsCParserStartNode& aNode,
                       nsTokenAllocator* aAllocator,
                       const nsAString& aAttrName,
                       const nsAString& aAttrValue);

protected:

    nsParser*           mParser;
    nsIHTMLContentSink* mSink;
    PRInt32             mLineNumber;
    nsITokenizer*       mTokenizer; 

    PRPackedBool        mSyntaxHighlight;
    PRPackedBool        mWrapLongLines;
    PRPackedBool        mHasOpenRoot;
    PRPackedBool        mHasOpenBody;

    nsDTDMode           mDTDMode;
    eParserCommands     mParserCommand;   
    eParserDocType      mDocType;
    nsCString           mMimeType;  

    nsString            mFilename;

    PRUint32            mTokenCount;
};

#endif 
