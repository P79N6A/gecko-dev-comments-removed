











































#ifndef nsCSSStruct_h___
#define nsCSSStruct_h___

#include "nsCSSValue.h"



struct nsCSSStruct {
  
};




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

struct nsCSSColor : public nsCSSStruct  {
  nsCSSColor(void);
  ~nsCSSColor(void);

  nsCSSValue mColor;
  nsCSSValue mBackColor;
  nsCSSValue mBackImage;
  nsCSSValue mBackRepeat;
  nsCSSValue mBackAttachment;
  nsCSSValue mBackPosition;
  nsCSSValue mBackSize;
  nsCSSValue mBackClip;
  nsCSSValue mBackOrigin;
  nsCSSValue mBackInlinePolicy;
private:
  nsCSSColor(const nsCSSColor& aOther); 
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
  nsCSSValue mTextShadow; 
  nsCSSValue mUnicodeBidi;  
  nsCSSValue mLineHeight;
  nsCSSValue mWhiteSpace;
  nsCSSValue mWordWrap;
private:
  nsCSSText(const nsCSSText& aOther); 
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
  nsCSSValue mTransform; 
  nsCSSValue mTransformOrigin;
  nsCSSValue mTransitionProperty;
  nsCSSValue mTransitionDuration;
  nsCSSValue mTransitionTimingFunction;
  nsCSSValue mTransitionDelay;

  
  nsCSSValue mBreakBefore;
  nsCSSValue mBreakAfter;
  
private:
  nsCSSDisplay(const nsCSSDisplay& aOther); 
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
  nsCSSRect   mBorderColors;
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
  nsCSSValue  mBoxShadow;
private:
  nsCSSMargin(const nsCSSMargin& aOther); 
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

struct nsCSSTable : public nsCSSStruct  { 
  nsCSSTable(void);
  ~nsCSSTable(void);

  nsCSSValue mBorderCollapse;
  nsCSSValue mBorderSpacing;
  nsCSSValue mCaptionSide;
  nsCSSValue mEmptyCells;

  nsCSSValue mLayout;
private:
  nsCSSTable(const nsCSSTable& aOther); 
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

struct nsCSSPage : public nsCSSStruct  { 
  nsCSSPage(void);
  ~nsCSSPage(void);

  nsCSSValue mMarks;
  nsCSSValue mSize;
private:
  nsCSSPage(const nsCSSPage& aOther); 
};

struct nsCSSContent : public nsCSSStruct  {
  nsCSSContent(void);
  ~nsCSSContent(void);

  nsCSSValue mContent;
  nsCSSValue mCounterIncrement;
  nsCSSValue mCounterReset;
  nsCSSValue mMarkerOffset;
  nsCSSValue mQuotes;
private:
  nsCSSContent(const nsCSSContent& aOther); 
};

struct nsCSSUserInterface : public nsCSSStruct  { 
  nsCSSUserInterface(void);
  ~nsCSSUserInterface(void);

  nsCSSValue mUserInput;
  nsCSSValue mUserModify;
  nsCSSValue mUserSelect;
  nsCSSValue mUserFocus;

  nsCSSValue mCursor;
  nsCSSValue mForceBrokenImageIcon;
  nsCSSValue mIMEMode;
  nsCSSValue mWindowShadow;
private:
  nsCSSUserInterface(const nsCSSUserInterface& aOther); 
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
  nsCSSValue mStrokeDasharray;
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

#endif 
