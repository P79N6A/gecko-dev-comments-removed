











































#ifndef nsCSSStruct_h___
#define nsCSSStruct_h___

#include "nsCSSValue.h"
#include <stdio.h>


struct nsCSSValueList {
  nsCSSValueList(void);
  nsCSSValueList(const nsCSSValueList& aCopy);
  ~nsCSSValueList(void);

  static PRBool Equal(nsCSSValueList* aList1, nsCSSValueList* aList2);

  nsCSSValue      mValue;
  nsCSSValueList* mNext;
};

struct nsCSSRect {
  nsCSSRect(void);
  nsCSSRect(const nsCSSRect& aCopy);
  ~nsCSSRect();

  PRBool operator==(const nsCSSRect& aOther) const {
    return mTop == aOther.mTop &&
           mRight == aOther.mRight &&
           mBottom == aOther.mBottom &&
           mLeft == aOther.mLeft;
  }

  PRBool operator!=(const nsCSSRect& aOther) const {
    return mTop != aOther.mTop ||
           mRight != aOther.mRight ||
           mBottom != aOther.mBottom ||
           mLeft != aOther.mLeft;
  }

  void SetAllSidesTo(const nsCSSValue& aValue);

  void Reset() {
    mTop.Reset();
    mRight.Reset();
    mBottom.Reset();
    mLeft.Reset();
  }

  PRBool HasValue() const {
    return
      mTop.GetUnit() != eCSSUnit_Null ||
      mRight.GetUnit() != eCSSUnit_Null ||
      mBottom.GetUnit() != eCSSUnit_Null ||
      mLeft.GetUnit() != eCSSUnit_Null;
  }
  
  nsCSSValue mTop;
  nsCSSValue mRight;
  nsCSSValue mBottom;
  nsCSSValue mLeft;

  typedef nsCSSValue nsCSSRect::*side_type;
  static const side_type sides[4];
};

struct nsCSSValuePair {
  nsCSSValuePair()
  {
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  nsCSSValuePair(const nsCSSValuePair& aCopy)
    : mXValue(aCopy.mXValue),
      mYValue(aCopy.mYValue)
  { 
    MOZ_COUNT_CTOR(nsCSSValuePair);
  }
  ~nsCSSValuePair()
  {
    MOZ_COUNT_DTOR(nsCSSValuePair);
  }

  PRBool operator==(const nsCSSValuePair& aOther) const {
    return mXValue == aOther.mXValue &&
           mYValue == aOther.mYValue;
  }

  PRBool operator!=(const nsCSSValuePair& aOther) const {
    return mXValue != aOther.mXValue ||
           mYValue != aOther.mYValue;
  }

  void SetBothValuesTo(const nsCSSValue& aValue) {
    mXValue = aValue;
    mYValue = aValue;
  }

  nsCSSValue mXValue;
  nsCSSValue mYValue;
};

struct nsCSSValueListRect {
  nsCSSValueListRect(void);
  nsCSSValueListRect(const nsCSSValueListRect& aCopy);
  ~nsCSSValueListRect();

  nsCSSValueList* mTop;
  nsCSSValueList* mRight;
  nsCSSValueList* mBottom;
  nsCSSValueList* mLeft;

  typedef nsCSSValueList* nsCSSValueListRect::*side_type;
  static const side_type sides[4];
};


struct nsCSSCounterData {
  nsCSSCounterData(void);
  nsCSSCounterData(const nsCSSCounterData& aCopy);
  ~nsCSSCounterData(void);

  static PRBool Equal(nsCSSCounterData* aList1, nsCSSCounterData* aList2);

  nsCSSValue        mCounter;
  nsCSSValue        mValue;
  nsCSSCounterData* mNext;
};


struct nsCSSQuotes {
  nsCSSQuotes(void);
  nsCSSQuotes(const nsCSSQuotes& aCopy);
  ~nsCSSQuotes(void);

  static PRBool Equal(nsCSSQuotes* aList1, nsCSSQuotes* aList2);

  nsCSSValue    mOpen;
  nsCSSValue    mClose;
  nsCSSQuotes*  mNext;
};



struct nsCSSStruct {
  
};









typedef nsCSSStruct nsRuleDataStruct;


struct nsCSSFont : public nsCSSStruct {
  nsCSSFont(void);
  ~nsCSSFont(void);

  nsCSSValue mSystemFont;
  nsCSSValue mFamily;
  nsCSSValue mStyle;
  nsCSSValue mVariant;
  nsCSSValue mWeight;
  nsCSSValue mSize;
  nsCSSValue mSizeAdjust; 
  nsCSSValue mStretch; 

private:
  nsCSSFont(const nsCSSFont& aOther); 
};

struct nsRuleDataFont : public nsCSSFont {
  PRBool mFamilyFromHTML; 
  nsRuleDataFont() {}
private:
  nsRuleDataFont(const nsRuleDataFont& aOther); 
};

struct nsCSSColor : public nsCSSStruct  {
  nsCSSColor(void);
  ~nsCSSColor(void);

  nsCSSValue      mColor;
  nsCSSValue      mBackColor;
  nsCSSValue      mBackImage;
  nsCSSValue      mBackRepeat;
  nsCSSValue      mBackAttachment;
  nsCSSValuePair  mBackPosition;
  nsCSSValue      mBackClip;
  nsCSSValue      mBackOrigin;
  nsCSSValue      mBackInlinePolicy;
private:
  nsCSSColor(const nsCSSColor& aOther); 
};

struct nsRuleDataColor : public nsCSSColor {
  nsRuleDataColor() {}
private:
  nsRuleDataColor(const nsRuleDataColor& aOther); 
};

struct nsCSSText : public nsCSSStruct  {
  nsCSSText(void);
  ~nsCSSText(void);

  nsCSSValue mWordSpacing;
  nsCSSValue mLetterSpacing;
  nsCSSValue mVerticalAlign;
  nsCSSValue mTextTransform;
  nsCSSValue mTextAlign;
  nsCSSValue mTextIndent;
  nsCSSValue mDecoration;
  nsCSSValueList* mTextShadow; 
  nsCSSValue mUnicodeBidi;  
  nsCSSValue mLineHeight;
  nsCSSValue mWhiteSpace;
private:
  nsCSSText(const nsCSSText& aOther); 
};

struct nsRuleDataText : public nsCSSText {
  nsRuleDataText() {}
private:
  nsRuleDataText(const nsRuleDataText& aOther); 
};

struct nsCSSDisplay : public nsCSSStruct  {
  nsCSSDisplay(void);
  ~nsCSSDisplay(void);

  nsCSSValue mDirection;
  nsCSSValue mDisplay;
  nsCSSValue mBinding;
  nsCSSValue mAppearance;
  nsCSSValue mPosition;
  nsCSSValue mFloat;
  nsCSSValue mClear;
  nsCSSRect  mClip;
  nsCSSValue mOverflowX;
  nsCSSValue mOverflowY;
  nsCSSValue mVisibility;
  nsCSSValue mOpacity;

  
  nsCSSValue mBreakBefore;
  nsCSSValue mBreakAfter;
  
private:
  nsCSSDisplay(const nsCSSDisplay& aOther); 
};

struct nsRuleDataDisplay : public nsCSSDisplay {
  nsCSSValue mLang;
  nsRuleDataDisplay() {}
private:
  nsRuleDataDisplay(const nsRuleDataDisplay& aOther); 
};

struct nsCSSMargin : public nsCSSStruct  {
  nsCSSMargin(void);
  ~nsCSSMargin(void);

  nsCSSRect   mMargin;
  nsCSSValue  mMarginStart;
  nsCSSValue  mMarginEnd;
  nsCSSValue  mMarginLeftLTRSource;
  nsCSSValue  mMarginLeftRTLSource;
  nsCSSValue  mMarginRightLTRSource;
  nsCSSValue  mMarginRightRTLSource;
  nsCSSRect   mPadding;
  nsCSSValue  mPaddingStart;
  nsCSSValue  mPaddingEnd;
  nsCSSValue  mPaddingLeftLTRSource;
  nsCSSValue  mPaddingLeftRTLSource;
  nsCSSValue  mPaddingRightLTRSource;
  nsCSSValue  mPaddingRightRTLSource;
  nsCSSRect   mBorderWidth;
  nsCSSValue  mBorderStartWidth;
  nsCSSValue  mBorderEndWidth;
  nsCSSValue  mBorderLeftWidthLTRSource;
  nsCSSValue  mBorderLeftWidthRTLSource;
  nsCSSValue  mBorderRightWidthLTRSource;
  nsCSSValue  mBorderRightWidthRTLSource;
  nsCSSRect   mBorderColor;
  nsCSSValue  mBorderStartColor;
  nsCSSValue  mBorderEndColor;
  nsCSSValue  mBorderLeftColorLTRSource;
  nsCSSValue  mBorderLeftColorRTLSource;
  nsCSSValue  mBorderRightColorLTRSource;
  nsCSSValue  mBorderRightColorRTLSource;
  nsCSSValueListRect mBorderColors;
  nsCSSRect   mBorderStyle;
  nsCSSValue  mBorderStartStyle;
  nsCSSValue  mBorderEndStyle;
  nsCSSValue  mBorderLeftStyleLTRSource;
  nsCSSValue  mBorderLeftStyleRTLSource;
  nsCSSValue  mBorderRightStyleLTRSource;
  nsCSSValue  mBorderRightStyleRTLSource;
  nsCSSRect   mBorderRadius;  
  nsCSSValue  mOutlineWidth;
  nsCSSValue  mOutlineColor;
  nsCSSValue  mOutlineStyle;
  nsCSSValue  mOutlineOffset;
  nsCSSRect   mOutlineRadius; 
  nsCSSValue  mFloatEdge; 
private:
  nsCSSMargin(const nsCSSMargin& aOther); 
};

struct nsRuleDataMargin : public nsCSSMargin {
  nsRuleDataMargin() {}
private:
  nsRuleDataMargin(const nsRuleDataMargin& aOther); 
};

struct nsCSSPosition : public nsCSSStruct  {
  nsCSSPosition(void);
  ~nsCSSPosition(void);

  nsCSSValue  mWidth;
  nsCSSValue  mMinWidth;
  nsCSSValue  mMaxWidth;
  nsCSSValue  mHeight;
  nsCSSValue  mMinHeight;
  nsCSSValue  mMaxHeight;
  nsCSSValue  mBoxSizing; 
  nsCSSRect   mOffset;
  nsCSSValue  mZIndex;
private:
  nsCSSPosition(const nsCSSPosition& aOther); 
};

struct nsRuleDataPosition : public nsCSSPosition {
  nsRuleDataPosition() {}
private:
  nsRuleDataPosition(const nsRuleDataPosition& aOther); 
};

struct nsCSSList : public nsCSSStruct  {
  nsCSSList(void);
  ~nsCSSList(void);

  nsCSSValue mType;
  nsCSSValue mImage;
  nsCSSValue mPosition;
  nsCSSRect  mImageRegion;
private:
  nsCSSList(const nsCSSList& aOther); 
};

struct nsRuleDataList : public nsCSSList {
  nsRuleDataList() {}
private:
  nsRuleDataList(const nsRuleDataList& aOther); 
};

struct nsCSSTable : public nsCSSStruct  { 
  nsCSSTable(void);
  ~nsCSSTable(void);

  nsCSSValue mBorderCollapse;
  nsCSSValuePair mBorderSpacing;
  nsCSSValue mCaptionSide;
  nsCSSValue mEmptyCells;
  
  nsCSSValue mLayout;
  nsCSSValue mFrame; 
  nsCSSValue mRules; 
  nsCSSValue mSpan; 
  nsCSSValue mCols; 
private:
  nsCSSTable(const nsCSSTable& aOther); 
};

struct nsRuleDataTable : public nsCSSTable {
  nsRuleDataTable() {}
private:
  nsRuleDataTable(const nsRuleDataTable& aOther); 
};

struct nsCSSBreaks : public nsCSSStruct  { 
  nsCSSBreaks(void);
  ~nsCSSBreaks(void);

  nsCSSValue mOrphans;
  nsCSSValue mWidows;
  nsCSSValue mPage;
  
  
  
  nsCSSValue mPageBreakInside;
private:
  nsCSSBreaks(const nsCSSBreaks& aOther); 
};

struct nsRuleDataBreaks : public nsCSSBreaks {
  nsRuleDataBreaks() {}
private:
  nsRuleDataBreaks(const nsRuleDataBreaks& aOther); 
};

struct nsCSSPage : public nsCSSStruct  { 
  nsCSSPage(void);
  ~nsCSSPage(void);

  nsCSSValue mMarks;
  nsCSSValuePair mSize;
private:
  nsCSSPage(const nsCSSPage& aOther); 
};

struct nsRuleDataPage : public nsCSSPage {
  nsRuleDataPage() {}
private:
  nsRuleDataPage(const nsRuleDataPage& aOther); 
};

struct nsCSSContent : public nsCSSStruct  {
  nsCSSContent(void);
  ~nsCSSContent(void);

  nsCSSValueList*   mContent;
  nsCSSCounterData* mCounterIncrement;
  nsCSSCounterData* mCounterReset;
  nsCSSValue        mMarkerOffset;
  nsCSSQuotes*      mQuotes;
private:
  nsCSSContent(const nsCSSContent& aOther); 
};

struct nsRuleDataContent : public nsCSSContent {
  nsRuleDataContent() {}
private:
  nsRuleDataContent(const nsRuleDataContent& aOther); 
};

struct nsCSSUserInterface : public nsCSSStruct  { 
  nsCSSUserInterface(void);
  ~nsCSSUserInterface(void);

  nsCSSValue      mUserInput;
  nsCSSValue      mUserModify;
  nsCSSValue      mUserSelect;
  nsCSSValue      mUserFocus;
  
  nsCSSValueList* mCursor;
  nsCSSValue      mForceBrokenImageIcon;
  nsCSSValue      mIMEMode;
private:
  nsCSSUserInterface(const nsCSSUserInterface& aOther); 
};

struct nsRuleDataUserInterface : public nsCSSUserInterface {
  nsRuleDataUserInterface() {}
private:
  nsRuleDataUserInterface(const nsRuleDataUserInterface& aOther); 
};

struct nsCSSAural : public nsCSSStruct  { 
  nsCSSAural(void);
  ~nsCSSAural(void);

  nsCSSValue mAzimuth;
  nsCSSValue mElevation;
  nsCSSValue mCueAfter;
  nsCSSValue mCueBefore;
  nsCSSValue mPauseAfter;
  nsCSSValue mPauseBefore;
  nsCSSValue mPitch;
  nsCSSValue mPitchRange;
  nsCSSValuePair mPlayDuring; 
  nsCSSValue mRichness;
  nsCSSValue mSpeak;
  nsCSSValue mSpeakHeader;
  nsCSSValue mSpeakNumeral;
  nsCSSValue mSpeakPunctuation;
  nsCSSValue mSpeechRate;
  nsCSSValue mStress;
  nsCSSValue mVoiceFamily;
  nsCSSValue mVolume;
private:
  nsCSSAural(const nsCSSAural& aOther); 
};

struct nsRuleDataAural : public nsCSSAural {
  nsRuleDataAural() {}
private:
  nsRuleDataAural(const nsRuleDataAural& aOther); 
};

struct nsCSSXUL : public nsCSSStruct  {
  nsCSSXUL(void);
  ~nsCSSXUL(void);

  nsCSSValue  mBoxAlign;
  nsCSSValue  mBoxDirection;
  nsCSSValue  mBoxFlex;
  nsCSSValue  mBoxOrient;
  nsCSSValue  mBoxPack;
  nsCSSValue  mBoxOrdinal;
private:
  nsCSSXUL(const nsCSSXUL& aOther); 
};

struct nsRuleDataXUL : public nsCSSXUL {
  nsRuleDataXUL() {}
private:
  nsRuleDataXUL(const nsRuleDataXUL& aOther); 
};

struct nsCSSColumn : public nsCSSStruct  {
  nsCSSColumn(void);
  ~nsCSSColumn(void);

  nsCSSValue  mColumnCount;
  nsCSSValue  mColumnWidth;
  nsCSSValue  mColumnGap;
private:
  nsCSSColumn(const nsCSSColumn& aOther); 
};

struct nsRuleDataColumn : public nsCSSColumn {
  nsRuleDataColumn() {}
private:
  nsRuleDataColumn(const nsRuleDataColumn& aOther); 
};

#ifdef MOZ_SVG
struct nsCSSSVG : public nsCSSStruct {
  nsCSSSVG(void);
  ~nsCSSSVG(void);

  nsCSSValue mClipPath;
  nsCSSValue mClipRule;
  nsCSSValue mColorInterpolation;
  nsCSSValue mColorInterpolationFilters;
  nsCSSValue mDominantBaseline;
  nsCSSValuePair mFill;
  nsCSSValue mFillOpacity;
  nsCSSValue mFillRule;
  nsCSSValue mFilter;
  nsCSSValue mFloodColor;
  nsCSSValue mFloodOpacity;
  nsCSSValue mMarkerEnd;
  nsCSSValue mMarkerMid;
  nsCSSValue mMarkerStart;
  nsCSSValue mMask;
  nsCSSValue mPointerEvents;
  nsCSSValue mShapeRendering;
  nsCSSValue mStopColor;
  nsCSSValue mStopOpacity;
  nsCSSValuePair mStroke;
  nsCSSValueList *mStrokeDasharray;
  nsCSSValue mStrokeDashoffset;
  nsCSSValue mStrokeLinecap;
  nsCSSValue mStrokeLinejoin;
  nsCSSValue mStrokeMiterlimit;
  nsCSSValue mStrokeOpacity;
  nsCSSValue mStrokeWidth;
  nsCSSValue mTextAnchor;
  nsCSSValue mTextRendering;
private:
  nsCSSSVG(const nsCSSSVG& aOther); 
};

struct nsRuleDataSVG : public nsCSSSVG {
  nsRuleDataSVG() {}
private:
  nsRuleDataSVG(const nsRuleDataSVG& aOther); 
};
#endif

#endif 
