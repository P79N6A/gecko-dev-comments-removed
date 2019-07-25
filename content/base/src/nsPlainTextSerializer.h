










































#ifndef nsPlainTextSerializer_h__
#define nsPlainTextSerializer_h__

#include "nsIContentSerializer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsILineBreaker.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDocumentEncoder.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsPlainTextSerializer : public nsIContentSerializer
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

protected:
  nsresult GetAttributeValue(nsIAtom* aName, nsString& aValueRet);
  void AddToLine(const PRUnichar* aStringToAdd, PRInt32 aLength);
  void EndLine(bool softlinebreak, bool aBreakBySpace = false);
  void EnsureVerticalSpace(PRInt32 noOfRows);
  void FlushLine();
  void OutputQuotesAndIndent(bool stripTrailingSpaces=false);
  void Output(nsString& aString);
  void Write(const nsAString& aString);
  bool IsInPre();
  bool IsInOL();
  bool IsCurrentNodeConverted();
  bool MustSuppressLeaf();

  



  static nsIAtom* GetIdForContent(nsIContent* aContent);
  nsresult DoOpenContainer(nsIAtom* aTag);
  nsresult DoCloseContainer(nsIAtom* aTag);
  nsresult DoAddLeaf(nsIAtom* aTag);
  void DoAddText(bool aIsWhitespace, const nsAString& aText);

  
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
  
  
  nsAutoTArray<bool, 8> mIsInCiteBlockquote;

  
  nsAString*            mOutputString;

  
  
  
  nsIAtom**        mTagStack;
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
