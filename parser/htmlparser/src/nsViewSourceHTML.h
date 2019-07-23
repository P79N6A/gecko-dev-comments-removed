











































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

class CIndirectTextToken : public CTextToken {
public:
  CIndirectTextToken() : CTextToken() {
    mIndirectString=0;
  }

  void SetIndirectString(const nsSubstring& aString) {
    mIndirectString=&aString;
  }

  virtual const nsSubstring& GetStringValue(void){
    return (const nsSubstring&)*mIndirectString;
  }

  const nsSubstring* mIndirectString;
};


class CViewSourceHTML: public nsIDTD
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDTD

    CViewSourceHTML();
    virtual ~CViewSourceHTML();

    






    virtual void SetVerification(PRBool aEnable);

private:
    nsresult HandleToken(CToken* aToken, nsIParser* aParser);

    nsresult WriteTag(PRInt32 tagType,
                      const nsSubstring &aText,
                      PRInt32 attrCount,
                      PRBool aTagInError);

    nsresult WriteAttributes(const nsAString& tagName, 
        nsTokenAllocator* allocator, PRInt32 attrCount, PRBool aOwnerInError);
    void StartNewPreBlock(void);
    
    void AddAttrToNode(nsCParserStartNode& aNode,
                       nsTokenAllocator* aAllocator,
                       const nsAString& aAttrName,
                       const nsAString& aAttrValue);

    PRBool IsUrlAttribute(const nsAString& tagName,
                          const nsAString& attrName, const nsAString& attrValue);
    void WriteHrefAttribute(nsTokenAllocator* allocator, const nsAString& href);
    nsresult CreateViewSourceURL(const nsAString& linkUrl, nsString& viewSourceUrl);
    void WriteTextInSpan(const nsAString& text, nsTokenAllocator* allocator,
                         const nsAString& attrName, const nsAString& attrValue);
    void WriteTextInAnchor(const nsAString& text, nsTokenAllocator* allocator,
                           const nsAString& attrName, const nsAString &attrValue);
    void WriteTextInElement(const nsAString& tagName, eHTMLTags tagType,
                            const nsAString& text, nsTokenAllocator* allocator,
                            const nsAString& attrName, const nsAString& attrValue);
    const nsDependentSubstring TrimTokenValue(const nsAString& tokenValue);
    void TrimTokenValue(nsAString::const_iterator& start, 
                        nsAString::const_iterator& end);
    PRBool IsTokenValueTrimmableCharacter(PRUnichar ch);
    nsresult GetBaseURI(nsIURI **result);
    nsresult SetBaseURI(const nsAString& baseSpec);
    static void ExpandEntities(const nsAString& textIn, nsString& textOut);
    static void CopyPossibleEntity(nsAString::const_iterator& iter,
                                   const nsAString::const_iterator& end,
                                   nsAString& textBuffer);
    static PRInt32 ToUnicode(const nsString &strNum, PRInt32 radix, PRInt32 fallback);

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
    nsCOMPtr<nsIURI>    mBaseURI; 

    PRUint32            mTokenCount;

    nsCParserStartNode  mStartNode;
    nsCParserStartNode  mTokenNode;
    CIndirectTextToken  mITextToken;
    nsCParserStartNode  mErrorNode;
};

#endif
