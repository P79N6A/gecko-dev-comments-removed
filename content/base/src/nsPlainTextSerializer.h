










































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
#include "nsTArray.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsPlainTextSerializer : public nsIContentSerializer,
                              public nsIHTMLContentSink,
                              public nsIHTMLToTextSink
{
public:
  nsPlainTextSerializer();
  virtual ~nsPlainTextSerializer();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, bool aIsCopying,
                  bool aIsWholeDocument);

  NS_IMETHOD AppendText(nsIContent* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);
  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);
  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendComment(nsIContent* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendDoctype(nsIContent *aDoctype,
                           nsAString& aStr)  { return NS_OK; }
  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr); 
  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr);
  NS_IMETHOD Flush(nsAString& aStr);

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);

  
  NS_IMETHOD WillParse(void) { return NS_OK; }
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
  NS_IMETHOD IsEnabled(PRInt32 aTag, bool* aReturn);
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD_(bool) IsFormOnStack() { return false; }

  NS_IMETHOD BeginContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD EndContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD DidProcessTokens(void) { return NS_OK; }
  NS_IMETHOD WillProcessAToken(void) { return NS_OK; }
  NS_IMETHOD DidProcessAToken(void) { return NS_OK; }

  
  NS_IMETHOD Initialize(nsAString* aOutString,
                        PRUint32 aFlags, PRUint32 aWrapCol);

protected:
  nsresult GetAttributeValue(const nsIParserNode* node, nsIAtom* aName, nsString& aValueRet);
  void AddToLine(const PRUnichar* aStringToAdd, PRInt32 aLength);
  void EndLine(bool softlinebreak, bool aBreakBySpace = false);
  void EnsureVerticalSpace(PRInt32 noOfRows);
  void FlushLine();
  void OutputQuotesAndIndent(bool stripTrailingSpaces=false);
  void Output(nsString& aString);
  void Write(const nsAString& aString);
  bool IsBlockLevel(PRInt32 aId);
  bool IsContainer(PRInt32 aId);
  bool IsInPre();
  bool IsInOL();
  bool IsCurrentNodeConverted(const nsIParserNode* aNode);
  static PRInt32 GetIdForContent(nsIContent* aContent);
  nsresult DoOpenContainer(const nsIParserNode* aNode, PRInt32 aTag);
  nsresult DoCloseContainer(PRInt32 aTag);
  nsresult DoAddLeaf(const nsIParserNode* aNode,
                     PRInt32 aTag,
                     const nsAString& aText);

  
  inline bool MayWrap()
  {
    return mWrapColumn &&
      ((mFlags & nsIDocumentEncoder::OutputFormatted) ||
       (mFlags & nsIDocumentEncoder::OutputWrap));
  }

  inline bool DoOutput()
  {
    return mHeadLevel == 0;
  }

  
  bool GetLastBool(const nsTArray<bool>& aStack);
  void SetLastBool(nsTArray<bool>& aStack, bool aValue);
  void PushBool(nsTArray<bool>& aStack, bool aValue);
  bool PopBool(nsTArray<bool>& aStack);
  
protected:
  nsString         mCurrentLine;
  PRUint32         mHeadLevel;
  bool             mAtFirstColumn;

  
  
  
  
  
  
  
  
  bool             mQuotesPreformatted; 
  bool             mDontWrapAnyQuotes;  

  bool             mStructs;            

  
  
  
  bool             mHasWrittenCiteBlockquote;

  PRInt32          mIndent;
  
  
  nsString         mInIndentString;
  PRInt32          mCiteQuoteLevel;
  PRInt32          mFlags;
  PRInt32          mFloatingLines; 

  
  
  
  PRUint32         mWrapColumn;

  
  PRUint32         mCurrentLineWidth; 

  
  
  PRInt32          mSpanLevel;


  PRInt32          mEmptyLines; 
                                
                                

  bool             mInWhitespace;
  bool             mPreFormatted;
  bool             mStartedOutput; 

  
  
  
  bool             mLineBreakDue; 

  nsString         mURL;
  PRInt32          mHeaderStrategy;    




  PRInt32          mHeaderCounter[7];  





  nsRefPtr<mozilla::dom::Element> mElement;

  
  nsAutoTArray<bool, 8> mHasWrittenCellsForRow;
  
  
  nsAutoTArray<bool, 8> mCurrentNodeIsConverted;
  nsAutoTArray<bool, 8> mIsInCiteBlockquote;

  
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
