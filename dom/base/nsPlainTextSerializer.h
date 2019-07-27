










#ifndef nsPlainTextSerializer_h__
#define nsPlainTextSerializer_h__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIContentSerializer.h"
#include "nsIDocumentEncoder.h"
#include "nsILineBreaker.h"
#include "nsString.h"
#include "nsTArray.h"

#include <stack>

class nsIContent;

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsPlainTextSerializer MOZ_FINAL : public nsIContentSerializer
{
public:
  nsPlainTextSerializer();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(uint32_t flags, uint32_t aWrapColumn,
                  const char* aCharSet, bool aIsCopying,
                  bool aIsWholeDocument) MOZ_OVERRIDE;

  NS_IMETHOD AppendText(nsIContent* aText, int32_t aStartOffset,
                        int32_t aEndOffset, nsAString& aStr) MOZ_OVERRIDE;
  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                int32_t aStartOffset, int32_t aEndOffset,
                                nsAString& aStr) MOZ_OVERRIDE;
  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         int32_t aStartOffset,
                                         int32_t aEndOffset,
                                         nsAString& aStr) MOZ_OVERRIDE  { return NS_OK; }
  NS_IMETHOD AppendComment(nsIContent* aComment, int32_t aStartOffset,
                           int32_t aEndOffset, nsAString& aStr) MOZ_OVERRIDE  { return NS_OK; }
  NS_IMETHOD AppendDoctype(nsIContent *aDoctype,
                           nsAString& aStr) MOZ_OVERRIDE  { return NS_OK; }
  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr) MOZ_OVERRIDE; 
  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr) MOZ_OVERRIDE;
  NS_IMETHOD Flush(nsAString& aStr) MOZ_OVERRIDE;

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr) MOZ_OVERRIDE;

private:
  ~nsPlainTextSerializer();

  nsresult GetAttributeValue(nsIAtom* aName, nsString& aValueRet);
  void AddToLine(const char16_t* aStringToAdd, int32_t aLength);
  void EndLine(bool softlinebreak, bool aBreakBySpace = false);
  void EnsureVerticalSpace(int32_t noOfRows);
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

  bool ShouldReplaceContainerWithPlaceholder(nsIAtom* aTag);

  bool IsElementPreformatted(mozilla::dom::Element* aElement);
  bool IsElementBlock(mozilla::dom::Element* aElement);

private:
  nsString         mCurrentLine;
  uint32_t         mHeadLevel;
  bool             mAtFirstColumn;

  
  
  
  
  
  
  
  bool             mDontWrapAnyQuotes;  

  bool             mStructs;            

  
  
  
  bool             mHasWrittenCiteBlockquote;

  int32_t          mIndent;
  
  
  nsString         mInIndentString;
  int32_t          mCiteQuoteLevel;
  int32_t          mFlags;
  int32_t          mFloatingLines; 

  
  
  
  uint32_t         mWrapColumn;

  
  uint32_t         mCurrentLineWidth; 

  
  
  int32_t          mSpanLevel;


  int32_t          mEmptyLines; 
                                
                                

  bool             mInWhitespace;
  bool             mPreFormattedMail; 
                                      
  bool             mStartedOutput; 

  
  
  
  bool             mLineBreakDue;

  bool             mPreformattedBlockBoundary;

  nsString         mURL;
  int32_t          mHeaderStrategy;    




  int32_t          mHeaderCounter[7];  





  nsRefPtr<mozilla::dom::Element> mElement;

  
  nsAutoTArray<bool, 8> mHasWrittenCellsForRow;
  
  
  nsAutoTArray<bool, 8> mIsInCiteBlockquote;

  
  nsAString*            mOutputString;

  
  
  
  nsIAtom**        mTagStack;
  uint32_t         mTagStackIndex;

  
  
  
  std::stack<bool> mPreformatStack;

  
  uint32_t          mIgnoreAboveIndex;

  
  int32_t         *mOLStack;
  uint32_t         mOLStackIndex;

  uint32_t         mULCount;

  nsString                     mLineBreak;
  nsCOMPtr<nsILineBreaker>     mLineBreaker;

  
  
  const nsString          kSpace;

  
  
  
  
  
  
  
  uint32_t mIgnoredChildNodeLevel;
};

nsresult
NS_NewPlainTextSerializer(nsIContentSerializer** aSerializer);

#endif
