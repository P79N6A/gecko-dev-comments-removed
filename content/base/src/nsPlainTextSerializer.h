










































#ifndef nsPlainTextSerializer_h__
#define nsPlainTextSerializer_h__

#include "nsIContentSerializer.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLTags.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsILineBreaker.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIHTMLToTextSink.h"
#include "nsIDocumentEncoder.h"
#include "nsVoidArray.h"


class nsPlainTextSerializer : public nsIContentSerializer,
                              public nsIHTMLContentSink,
                              public nsIHTMLToTextSink
{
public:
  nsPlainTextSerializer();
  virtual ~nsPlainTextSerializer();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying);

  NS_IMETHOD AppendText(nsIDOMText* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);
  NS_IMETHOD AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);
  NS_IMETHOD AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendComment(nsIDOMComment* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendDoctype(nsIDOMDocumentType *aDoctype,
                           nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                nsIDOMElement *aOriginalElement,
                                nsAString& aStr); 
  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr);
  NS_IMETHOD Flush(nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);

  
  NS_IMETHOD WillTokenize(void) { return NS_OK; }
  NS_IMETHOD WillBuildModel(void) { return NS_OK; }
  NS_IMETHOD DidBuildModel(void) { return NS_OK; }
  NS_IMETHOD WillInterrupt(void) { return NS_OK; }
  NS_IMETHOD WillResume(void) { return NS_OK; }
  NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode) { return NS_OK; }
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return nsnull; }

  
  NS_IMETHOD OpenHead();
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn);
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD_(PRBool) IsFormOnStack() { return PR_FALSE; }

  NS_IMETHOD BeginContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD EndContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD WillProcessTokens(void) { return NS_OK; }
  NS_IMETHOD DidProcessTokens(void) { return NS_OK; }
  NS_IMETHOD WillProcessAToken(void) { return NS_OK; }
  NS_IMETHOD DidProcessAToken(void) { return NS_OK; }

  
  NS_IMETHOD Initialize(nsAString* aOutString,
                        PRUint32 aFlags, PRUint32 aWrapCol);

protected:
  nsresult GetAttributeValue(const nsIParserNode* node, nsIAtom* aName, nsString& aValueRet);
  void AddToLine(const PRUnichar* aStringToAdd, PRInt32 aLength);
  void EndLine(PRBool softlinebreak);
  void EnsureVerticalSpace(PRInt32 noOfRows);
  void FlushLine();
  void OutputQuotesAndIndent(PRBool stripTrailingSpaces=PR_FALSE);
  void Output(nsString& aString);
  void Write(const nsAString& aString);
  PRBool IsBlockLevel(PRInt32 aId);
  PRBool IsContainer(PRInt32 aId);
  PRBool IsInPre();
  PRBool IsInOL();
  PRBool IsCurrentNodeConverted(const nsIParserNode* aNode);
  static PRInt32 GetIdForContent(nsIContent* aContent);
  nsresult DoOpenContainer(const nsIParserNode* aNode, PRInt32 aTag);
  nsresult DoCloseContainer(PRInt32 aTag);
  nsresult DoAddLeaf(const nsIParserNode* aNode,
                     PRInt32 aTag,
                     const nsAString& aText);

  
  inline PRBool MayWrap()
  {
    return mWrapColumn &&
      ((mFlags & nsIDocumentEncoder::OutputFormatted) ||
       (mFlags & nsIDocumentEncoder::OutputWrap));
  }

  inline PRBool DoOutput()
  {
    return !mInHead;
  }

  
  PRBool GetLastBool(const nsVoidArray& aStack);
  void SetLastBool(nsVoidArray& aStack, PRBool aValue);
  void PushBool(nsVoidArray& aStack, PRBool aValue);
  PRBool PopBool(nsVoidArray& aStack);
  
protected:
  nsString         mCurrentLine;

  PRPackedBool     mInHead;
  PRPackedBool     mAtFirstColumn;

  
  
  
  
  
  
  
  
  PRPackedBool     mQuotesPreformatted; 
  PRPackedBool     mDontWrapAnyQuotes;  

  PRPackedBool     mStructs;            

  
  
  
  PRPackedBool     mHasWrittenCiteBlockquote;

  PRInt32          mIndent;
  
  
  nsString         mInIndentString;
  PRInt32          mCiteQuoteLevel;
  PRInt32          mFlags;
  PRInt32          mFloatingLines; 

  
  
  
  PRUint32         mWrapColumn;

  
  PRUint32         mCurrentLineWidth; 

  
  
  PRInt32          mSpanLevel;


  PRInt32          mEmptyLines; 
                                
                                

  PRPackedBool     mInWhitespace;
  PRPackedBool     mPreFormatted;
  PRPackedBool     mStartedOutput; 

  
  
  
  PRPackedBool     mLineBreakDue; 

  nsString         mURL;
  PRInt32          mHeaderStrategy;    




  PRInt32          mHeaderCounter[7];  





  nsCOMPtr<nsIContent> mContent;

  
  nsAutoVoidArray mHasWrittenCellsForRow; 
  
  
  nsAutoVoidArray     mCurrentNodeIsConverted; 
  nsAutoVoidArray     mIsInCiteBlockquote; 

  
  nsAString*            mOutputString;

  
  nsHTMLTag       *mTagStack;
  PRUint32         mTagStackIndex;

  
  PRUint32          mIgnoreAboveIndex;

  
  PRInt32         *mOLStack;
  PRUint32         mOLStackIndex;

  PRUint32         mULCount;

  nsString                     mLineBreak;
  nsCOMPtr<nsILineBreaker>     mLineBreaker;

  
  
  const nsString          kSpace;
};

nsresult
NS_NewPlainTextSerializer(nsIContentSerializer** aSerializer);

#endif
