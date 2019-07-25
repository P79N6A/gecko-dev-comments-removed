











































#ifndef nsCSSStruct_h___
#define nsCSSStruct_h___

#include "nsCSSValue.h"
#include "nsStyleConsts.h"

struct nsCSSCornerSizes {
  nsCSSCornerSizes(void);
  nsCSSCornerSizes(const nsCSSCornerSizes& aCopy);
  ~nsCSSCornerSizes();

  
  nsCSSValue const & GetCorner(PRUint32 aCorner) const {
    return this->*corners[aCorner];
  }
  nsCSSValue & GetCorner(PRUint32 aCorner) {
    return this->*corners[aCorner];
  }

  PRBool operator==(const nsCSSCornerSizes& aOther) const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner) != aOther.GetCorner(corner))
        return PR_FALSE;
    }
    return PR_TRUE;
  }

  PRBool operator!=(const nsCSSCornerSizes& aOther) const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner) != aOther.GetCorner(corner))
        return PR_TRUE;
    }
    return PR_FALSE;
  }

  PRBool HasValue() const {
    NS_FOR_CSS_FULL_CORNERS(corner) {
      if (this->GetCorner(corner).GetUnit() != eCSSUnit_Null)
        return PR_TRUE;
    }
    return PR_FALSE;
  }

  void Reset();

  nsCSSValue mTopLeft;
  nsCSSValue mTopRight;
  nsCSSValue mBottomRight;
  nsCSSValue mBottomLeft;

protected:
  typedef nsCSSValue nsCSSCornerSizes::*corner_type;
  static const corner_type corners[4];
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
  nsCSSValue mFontFeatureSettings;
  nsCSSValue mFontLanguageOverride;

#ifdef MOZ_MATHML
  nsCSSValue mScriptLevel; 
  nsCSSValue mScriptSizeMultiplier;
  nsCSSValue mScriptMinSize;
#endif

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
  nsCSSValueList* mBackImage;
  nsCSSValueList* mBackRepeat;
  nsCSSValueList* mBackAttachment;
  nsCSSValuePairList* mBackPosition;
  nsCSSValuePairList* mBackSize;
  nsCSSValueList* mBackClip;
  nsCSSValueList* mBackOrigin;
  nsCSSValue      mBackInlinePolicy;
private:
  nsCSSColor(const nsCSSColor& aOther); 
};

struct nsRuleDataColor : public nsCSSColor {
  nsRuleDataColor() {}

  
  
  
  nsCSSValueList mTempBackImage;
private:
  nsRuleDataColor(const nsRuleDataColor& aOther); 
};

struct nsCSSText : public nsCSSStruct  {
  nsCSSText(void);
  ~nsCSSText(void);

  nsCSSValue mTabSize;
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
  nsCSSValue mWordWrap;
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
  nsCSSValue mClip;
  nsCSSValue mOverflowX;
  nsCSSValue mOverflowY;
  nsCSSValue mResize;
  nsCSSValue mPointerEvents;
  nsCSSValue mVisibility;
  nsCSSValue mOpacity;
  nsCSSValueList *mTransform; 
  nsCSSValue mTransformOrigin;
  nsCSSValueList* mTransitionProperty;
  nsCSSValueList* mTransitionDuration;
  nsCSSValueList* mTransitionTimingFunction;
  nsCSSValueList* mTransitionDelay;

  
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
  nsCSSCornerSizes mBorderRadius;
  nsCSSValue  mOutlineWidth;
  nsCSSValue  mOutlineColor;
  nsCSSValue  mOutlineStyle;
  nsCSSValue  mOutlineOffset;
  nsCSSCornerSizes mOutlineRadius;
  nsCSSValue  mFloatEdge; 
  nsCSSValue  mBorderImage;
  nsCSSValueList* mBoxShadow;
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
  nsCSSValue mImageRegion;
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
  nsCSSValue mBorderSpacing;
  nsCSSValue mCaptionSide;
  nsCSSValue mEmptyCells;
  
  nsCSSValue mLayout;
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
  nsCSSValue mSize;
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

  nsCSSValueList*     mContent;
  nsCSSValuePairList* mCounterIncrement;
  nsCSSValuePairList* mCounterReset;
  nsCSSValue          mMarkerOffset;
  nsCSSValuePairList* mQuotes;
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
  nsCSSValue      mWindowShadow;
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
  nsCSSValue  mStackSizing;
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
  nsCSSValue  mColumnRuleColor;
  nsCSSValue  mColumnRuleWidth;
  nsCSSValue  mColumnRuleStyle;
private:
  nsCSSColumn(const nsCSSColumn& aOther); 
};

struct nsRuleDataColumn : public nsCSSColumn {
  nsRuleDataColumn() {}
private:
  nsRuleDataColumn(const nsRuleDataColumn& aOther); 
};

struct nsCSSSVG : public nsCSSStruct {
  nsCSSSVG(void);
  ~nsCSSSVG(void);

  nsCSSValue mClipPath;
  nsCSSValue mClipRule;
  nsCSSValue mColorInterpolation;
  nsCSSValue mColorInterpolationFilters;
  nsCSSValue mDominantBaseline;
  nsCSSValue mFill;
  nsCSSValue mFillOpacity;
  nsCSSValue mFillRule;
  nsCSSValue mFilter;
  nsCSSValue mFloodColor;
  nsCSSValue mFloodOpacity;
  nsCSSValue mImageRendering;
  nsCSSValue mLightingColor;
  nsCSSValue mMarkerEnd;
  nsCSSValue mMarkerMid;
  nsCSSValue mMarkerStart;
  nsCSSValue mMask;
  nsCSSValue mShapeRendering;
  nsCSSValue mStopColor;
  nsCSSValue mStopOpacity;
  nsCSSValue mStroke;
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
