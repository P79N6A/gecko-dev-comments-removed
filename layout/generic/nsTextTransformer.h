









































#ifndef nsTextTransformer_h___
#define nsTextTransformer_h___

#include "nsTextFragment.h"
#include "nsISupports.h"
#include "nsPresContext.h"
#include "nsIObserver.h"
#ifdef IBMBIDI
#include "nsBidi.h"
#include "nsBidiUtils.h"
#endif

class nsIContent;
class nsIFrame;
class nsILineBreaker;
class nsIWordBreaker;
class nsICaseConversion;


#define CH_NBSP   160
#define CH_ENSP   8194 //<!ENTITY ensp    CDATA "&#8194;" -- en space, U+2002 ISOpub -->
#define CH_EMSP   8195 //<!ENTITY emsp    CDATA "&#8195;" -- em space, U+2003 ISOpub -->
#define CH_THINSP 8291 //<!ENTITY thinsp  CDATA "&#8201;" -- thin space, U+2009 ISOpub -->
#define CH_SHY    173
#define CH_CJKSP  12288 // U+3000 IDEOGRAPHIC SPACE (CJK Full-Width Space)

#ifdef IBMBIDI
#define CH_LRM  8206  //<!ENTITY lrm     CDATA "&#8206;" -- left-to-right mark, U+200E NEW RFC 2070 -->
#define CH_RLM  8207  //<!ENTITY rlm     CDATA "&#8207;" -- right-to-left mark, U+200F NEW RFC 2070 -->
#define CH_LRE  8234  //<!CDATA "&#8234;" -- left-to-right embedding, U+202A -->
#define CH_RLE  8235  //<!CDATA "&#8235;" -- right-to-left embedding, U+202B -->
#define CH_PDF  8236  //<!CDATA "&#8236;" -- pop directional format, U+202C -->
#define CH_LRO  8237  //<!CDATA "&#8237;" -- left-to-right override, U+202D -->
#define CH_RLO  8238  //<!CDATA "&#8238;" -- right-to-left override, U+202E -->

#define IS_BIDI_CONTROL(_ch) \
  (((_ch) >= CH_LRM && (_ch) <= CH_RLM) \
  || ((_ch) >= CH_LRE && (_ch) <= CH_RLO))
#endif 




#ifdef IBMBIDI

#define IS_DISCARDED(_ch) \
  (((_ch) == CH_SHY) || ((_ch) == '\r') || IS_BIDI_CONTROL(_ch))
#else
#define IS_DISCARDED(_ch) \
  (((_ch) == CH_SHY) || ((_ch) == '\r'))
#endif




#define IS_LAM(_ch) \
  (((_ch) == 0x0644) ||  /* ARABIC LETTER LAM */ \
   ((_ch) == 0xfedf) ||  /* ARABIC LETTER LAM INITIAL FORM */ \
   ((_ch) == 0xfee0))    /* ARABIC LETTER LAM MEDIAL FORM */  \




#define IS_ALEF(_ch) \
  (((_ch) == 0x0622) || /* ARABIC LETTER ALEF WITH MADDA ABOVE */ \
   ((_ch) == 0x0623) || /* ARABIC LETTER ALEF WITH HAMZA ABOVE */ \
   ((_ch) == 0x0625) || /* ARABIC LETTER ALEF WITH HAMZA BELOW */ \
   ((_ch) == 0x0627) || /* ARABIC LETTER ALEF */ \
   ((_ch) == 0xfe82) || /* ARABIC LETTER ALEF WITH MADDA ABOVE FINAL FORM */ \
   ((_ch) == 0xfe84) || /* ARABIC LETTER ALEF WITH HAMZA ABOVE FINAL FORM */ \
   ((_ch) == 0xfe88) || /* ARABIC LETTER ALEF WITH HAMZA BELOW FINAL FORM */ \
   ((_ch) == 0xfe8e))   /* ARABIC LETTER ALEF FINAL FORM */




#define IS_LAMALEF(_ch) (((_ch) >= 0xfef5) && ((_ch) <= 0xfefc))
  
  
  
  
  
  
  
  

#define IS_ASCII_CHAR(ch) ((ch&0xff80) == 0)

#define NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE 128 // used to be 256


#define NS_TEXT_TRANSFORMER_LEAVE_AS_ASCII					1



#define NS_TEXT_TRANSFORMER_HAS_MULTIBYTE					2


#define NS_TEXT_TRANSFORMER_TRANSFORMED_TEXT_IS_ASCII		4

#ifdef IBMBIDI

#define NS_TEXT_TRANSFORMER_DO_ARABIC_SHAPING 8


#define NS_TEXT_TRANSFORMER_DO_NUMERIC_SHAPING 16



#define NS_TEXT_TRANSFORMER_FRAME_IS_RTL 32
#endif



class nsAutoTextBuffer {
public:
  nsAutoTextBuffer();
  ~nsAutoTextBuffer();

  nsresult GrowBy(PRInt32 aAtLeast, PRBool aCopyToHead = PR_TRUE);

  nsresult GrowTo(PRInt32 aNewSize, PRBool aCopyToHead = PR_TRUE);

  PRUnichar* GetBuffer() { return mBuffer; }
  PRUnichar* GetBufferEnd() { return mBuffer + mBufferLen; }
  PRInt32 GetBufferLength() const { return mBufferLen; }

  PRUnichar* mBuffer;
  PRInt32 mBufferLen;
  PRUnichar mAutoBuffer[NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE];
};























class nsTextTransformer {
public:
  
  
  nsTextTransformer(nsPresContext* aPresContext);

  ~nsTextTransformer();

  











  nsresult Init(nsIFrame* aFrame,
                nsIContent* aContent,
                PRInt32 aStartingOffset,
                PRBool aForceArabicShaping = PR_FALSE,
                PRBool aLeaveAsAscii = PR_FALSE);

  PRInt32 GetContentLength() const {
    return mFrag ? mFrag->GetLength() : 0;
  }

  PRUnichar GetContentCharAt(PRInt32 aIndex) {
    return (mFrag && aIndex < mFrag->GetLength()) ? mFrag->CharAt(aIndex) : 0;
  }

  











  PRUnichar* GetNextWord(PRBool aInWord,
                         PRInt32* aWordLenResult,
                         PRInt32* aContentLenResult,
                         PRBool* aIsWhitespaceResult,
                         PRBool* aWasTransformed,
                         PRBool aResetTransformBuf = PR_TRUE,
                         PRBool aForLineBreak = PR_TRUE,
                         PRBool aIsKeyboardSelect = PR_FALSE);

  PRUnichar* GetPrevWord(PRBool aInWord,
                         PRInt32* aWordLenResult,
                         PRInt32* aContentLenResult,
                         PRBool* aIsWhitespaceResult,
                         PRBool aForLineBreak = PR_TRUE,
                         PRBool aIsKeyboardSelect = PR_FALSE);

  
  
  PRBool LeaveAsAscii() const {
      return (mFlags & NS_TEXT_TRANSFORMER_LEAVE_AS_ASCII) != 0;
  }

  
  PRBool HasMultibyte() const {
      return (mFlags & NS_TEXT_TRANSFORMER_HAS_MULTIBYTE) != 0;
  }

  
  
  PRBool TransformedTextIsAscii() const {
      return (mFlags & NS_TEXT_TRANSFORMER_TRANSFORMED_TEXT_IS_ASCII) != 0;
  }

#ifdef IBMBIDI
  
  
  PRBool NeedsArabicShaping() const {
    return (mFlags & NS_TEXT_TRANSFORMER_DO_ARABIC_SHAPING) != 0;
  }
  
  
  
  PRBool NeedsNumericShaping() const {
    return (mFlags & NS_TEXT_TRANSFORMER_DO_NUMERIC_SHAPING) != 0;
  }

  
  PRBool FrameIsRTL() const {
    return (mFlags & NS_TEXT_TRANSFORMER_FRAME_IS_RTL) != 0;
  }
#endif

  
  void SetLeaveAsAscii(PRBool aValue) {
      aValue ? mFlags |= NS_TEXT_TRANSFORMER_LEAVE_AS_ASCII : 
               mFlags &= (~NS_TEXT_TRANSFORMER_LEAVE_AS_ASCII);
  }
      
  
  void SetHasMultibyte(PRBool aValue) {
      aValue ? mFlags |= NS_TEXT_TRANSFORMER_HAS_MULTIBYTE : 
               mFlags &= (~NS_TEXT_TRANSFORMER_HAS_MULTIBYTE);
  }

  
  void SetTransformedTextIsAscii(PRBool aValue) {
      aValue ? mFlags |= NS_TEXT_TRANSFORMER_TRANSFORMED_TEXT_IS_ASCII : 
               mFlags &= (~NS_TEXT_TRANSFORMER_TRANSFORMED_TEXT_IS_ASCII);
  }

#ifdef IBMBIDI
  
  void SetNeedsArabicShaping(PRBool aValue) {
    aValue ? mFlags |= NS_TEXT_TRANSFORMER_DO_ARABIC_SHAPING : 
             mFlags &= (~NS_TEXT_TRANSFORMER_DO_ARABIC_SHAPING);
  }

  
  void SetNeedsNumericShaping(PRBool aValue) {
    aValue ? mFlags |= NS_TEXT_TRANSFORMER_DO_NUMERIC_SHAPING : 
                       mFlags &= (~NS_TEXT_TRANSFORMER_DO_NUMERIC_SHAPING);
  }

  
  void SetFrameIsRTL(PRBool aValue) {
    aValue ? mFlags |= NS_TEXT_TRANSFORMER_FRAME_IS_RTL:
             mFlags &= (~NS_TEXT_TRANSFORMER_FRAME_IS_RTL);
  }
#endif
  
  PRUnichar* GetWordBuffer() {
    return mTransformBuf.GetBuffer();
  }

  PRInt32 GetWordBufferLength() const {
    return mTransformBuf.GetBufferLength();
  }

  static PRBool GetWordSelectEatSpaceAfter() {
  	return sWordSelectEatSpaceAfter;
  }
  
  static PRBool GetWordSelectStopAtPunctuation() {
  	return sWordSelectStopAtPunctuation;
  }
  
  static nsICaseConversion* GetCaseConv();
  
  static nsresult Initialize();
  static void Shutdown();

protected:
  
  PRInt32 ScanNormalWhiteSpace_F(PRInt32 aFragLen);
  PRInt32 ScanNormalAsciiText_F(PRInt32  aFragLen,
                                PRInt32* aWordLen,
                                PRBool*  aWasTransformed);
  PRInt32 ScanNormalAsciiText_F_ForWordBreak(PRInt32  aFragLen,
                                             PRInt32* aWordLen,
                                             PRBool*  aWasTransformed,
                                             PRBool aIsKeyboardSelect);
  PRInt32 ScanNormalUnicodeText_F(PRInt32  aFragLen,
                                  PRBool   aForLineBreak,
                                  PRInt32* aWordLen,
                                  PRBool*  aWasTransformed);
  PRInt32 ScanPreWrapWhiteSpace_F(PRInt32  aFragLen,
                                  PRInt32* aWordLen);
  PRInt32 ScanPreAsciiData_F(PRInt32  aFragLen,
                             PRInt32* aWordLen,
                             PRBool*  aWasTransformed);
  PRInt32 ScanPreData_F(PRInt32  aFragLen,
                        PRInt32* aWordLen,
                        PRBool*  aWasTransformed);

  
  PRInt32 ScanNormalWhiteSpace_B();
  PRInt32 ScanNormalAsciiText_B(PRInt32* aWordLen, PRBool aIsKeyboardSelect);
  PRInt32 ScanNormalUnicodeText_B(PRBool aForLineBreak, PRInt32* aWordLen);
  PRInt32 ScanPreWrapWhiteSpace_B(PRInt32* aWordLen);
  PRInt32 ScanPreData_B(PRInt32* aWordLen);

  static nsresult EnsureCaseConv();

  
  
  void ConvertTransformedTextToUnicode();
  
  void LanguageSpecificTransform(PRUnichar* aText, PRInt32 aLen,
                                 PRBool* aWasTransformed);

  void DoArabicShaping(PRUnichar* aText, PRInt32& aTextLength, PRBool* aWasTransformed);

  void DoNumericShaping(PRUnichar* aText, PRInt32& aTextLength, PRBool* aWasTransformed);

  
  const nsTextFragment* mFrag;

  
  PRInt32 mOffset;

  
  enum {
    eNormal,
    ePreformatted,
    ePreWrap
  } mMode;
  
  nsLanguageSpecificTransformType mLanguageSpecificTransformType;

#ifdef IBMBIDI
  nsPresContext* mPresContext;
  nsCharType      mCharType;
#endif

  
  
  nsAutoTextBuffer mTransformBuf;

  
  
  PRInt32 mBufferPos;
  
  
  PRUint8 mTextTransform;

  
  PRUint8 mFlags;

  
  static int WordSelectPrefCallback(const char* aPref, void* aClosure);
  static PRBool sWordSelectListenerPrefChecked;  
  static PRBool sWordSelectEatSpaceAfter;        
  static PRBool sWordSelectStopAtPunctuation;    

  static nsICaseConversion* gCaseConv;

#ifdef DEBUG
  static void SelfTest(nsPresContext* aPresContext);

  nsresult Init2(const nsTextFragment* aFrag,
                 PRInt32 aStartingOffset,
                 PRUint8 aWhiteSpace,
                 PRUint8 aTextTransform);
#endif
};

#endif 
