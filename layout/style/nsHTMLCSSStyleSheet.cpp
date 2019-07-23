










































#include "nsIHTMLCSSStyleSheet.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsIURL.h"
#include "nsCSSPseudoElements.h"
#include "nsIStyleRule.h"
#include "nsIFrame.h"
#include "nsICSSStyleRule.h"
#include "nsIStyleRuleProcessor.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsCOMPtr.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"










class CSSDisablePropsRule : public nsIStyleRule {
public:
  CSSDisablePropsRule();
  virtual ~CSSDisablePropsRule();

  NS_DECL_ISUPPORTS

  
  
  void CommonMapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif
protected:
  nsCSSValueList mInheritList;
  nsCSSQuotes mInheritQuotes;
  nsCSSCounterData mNoneCounter;
};

CSSDisablePropsRule::CSSDisablePropsRule()
{
  nsCSSValue none(eCSSUnit_None);
  mNoneCounter.mCounter = none;
  nsCSSValue inherit(eCSSUnit_Inherit);
  mInheritList.mValue = inherit;
  mInheritQuotes.mOpen = inherit;
}

class CSSFirstLineRule : public CSSDisablePropsRule {
public:
  CSSFirstLineRule() {}

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

class CSSFirstLetterRule : public CSSDisablePropsRule {
public:
  CSSFirstLetterRule() {}

  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
};

CSSDisablePropsRule::~CSSDisablePropsRule()
{
}

NS_IMPL_ISUPPORTS1(CSSDisablePropsRule, nsIStyleRule)

#ifdef DEBUG
NS_IMETHODIMP
CSSDisablePropsRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}
#endif














void
CSSDisablePropsRule::CommonMapRuleInfoInto(nsRuleData* aData)
{
  




  
  if (aData->mSID == eStyleStruct_TextReset) {
    nsCSSValue normal(eCSSUnit_Normal);
    aData->mTextData->mUnicodeBidi = normal;
  }

  
  

  
  if (aData->mSID == eStyleStruct_Visibility) {
    nsCSSValue inherit(eCSSUnit_Inherit);
    aData->mDisplayData->mVisibility = inherit;
    aData->mDisplayData->mDirection = inherit;
  }

  if (aData->mSID == eStyleStruct_Display) {
    nsCSSValue none(eCSSUnit_None);
    aData->mDisplayData->mAppearance = none;

    nsCSSValue autovalue(eCSSUnit_Auto);
    aData->mDisplayData->mClip.mTop = autovalue;
    aData->mDisplayData->mClip.mRight = autovalue;
    aData->mDisplayData->mClip.mBottom = autovalue;
    aData->mDisplayData->mClip.mLeft = autovalue;

    nsCSSValue one(1.0f, eCSSUnit_Number);
    aData->mDisplayData->mOpacity = one;

    nsCSSValue inlinevalue(NS_STYLE_DISPLAY_INLINE, eCSSUnit_Enumerated);
    aData->mDisplayData->mDisplay = inlinevalue;

    aData->mDisplayData->mBinding = none;

    nsCSSValue staticposition(NS_STYLE_POSITION_STATIC, eCSSUnit_Enumerated);
    aData->mDisplayData->mPosition = staticposition;

    nsCSSValue visible(NS_STYLE_OVERFLOW_VISIBLE, eCSSUnit_Enumerated);
    aData->mDisplayData->mOverflowX = visible;
    aData->mDisplayData->mOverflowY = visible;

    aData->mDisplayData->mClear = none;

    
    
  }

  
  
  

  
  if (aData->mSID == eStyleStruct_Position) {
    nsCSSValue autovalue(eCSSUnit_Auto);
    nsCSSValue none(eCSSUnit_None);
    nsCSSValue zero(0.0f, eCSSUnit_Point);
    aData->mPositionData->mOffset.mTop = autovalue;
    aData->mPositionData->mOffset.mRight = autovalue;
    aData->mPositionData->mOffset.mBottom = autovalue;
    aData->mPositionData->mOffset.mLeft = autovalue;
    aData->mPositionData->mWidth = autovalue;
    aData->mPositionData->mMinWidth = zero;
    aData->mPositionData->mMaxWidth = none;
    aData->mPositionData->mHeight = autovalue;
    aData->mPositionData->mMinHeight = zero;
    aData->mPositionData->mMaxHeight = none;
    nsCSSValue content(NS_STYLE_BOX_SIZING_CONTENT, eCSSUnit_Enumerated);
    aData->mPositionData->mBoxSizing = content;
    aData->mPositionData->mZIndex = autovalue;
  }

  
  if (aData->mSID == eStyleStruct_Content) {
    

    aData->mContentData->mCounterIncrement = &mNoneCounter;
    aData->mContentData->mCounterReset = &mNoneCounter;

    nsCSSValue autovalue(eCSSUnit_Auto);
    aData->mContentData->mMarkerOffset = autovalue;
  }

  if (aData->mSID == eStyleStruct_Quotes) {
    aData->mContentData->mQuotes = &mInheritQuotes;
  }

  
  if (aData->mSID == eStyleStruct_UserInterface) {
    nsCSSValue inherit(eCSSUnit_Inherit);
    aData->mUserInterfaceData->mUserInput = inherit;
    aData->mUserInterfaceData->mUserModify = inherit;
    aData->mUserInterfaceData->mUserFocus = inherit;
    aData->mUserInterfaceData->mCursor = &mInheritList;
  }

  if (aData->mSID == eStyleStruct_UIReset) {
    nsCSSValue autovalue(eCSSUnit_Auto);
    nsCSSValue none(eCSSUnit_None);
    
    
    
    
  }

  
  if (aData->mSID == eStyleStruct_Outline) {
    nsCSSValue none(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
    aData->mMarginData->mOutlineStyle = none;
  }

}

NS_IMETHODIMP
CSSFirstLineRule::MapRuleInfoInto(nsRuleData* aData)
{
  









  CommonMapRuleInfoInto(aData);

  
  if (aData->mSID == eStyleStruct_Display) {
    nsCSSValue none(eCSSUnit_None);
    aData->mDisplayData->mFloat = none;
  }

  
  
  if (aData->mSID == eStyleStruct_Border) {
    nsCSSValue none(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
    aData->mMarginData->mBorderStyle.mTop = none;
    aData->mMarginData->mBorderStyle.mRight = none;
    aData->mMarginData->mBorderStyle.mBottom = none;
    aData->mMarginData->mBorderStyle.mLeft = none;
  }

  if (aData->mSID == eStyleStruct_Margin) {
    nsCSSValue zero(0.0f, eCSSUnit_Point);
    aData->mMarginData->mMargin.mTop = zero;
    aData->mMarginData->mMargin.mRight = zero;
    aData->mMarginData->mMargin.mBottom = zero;
    aData->mMarginData->mMargin.mLeft = zero;
  }

  if (aData->mSID == eStyleStruct_Padding) {
    nsCSSValue zero(0.0f, eCSSUnit_Point);
    aData->mMarginData->mPadding.mTop = zero;
    aData->mMarginData->mPadding.mRight = zero;
    aData->mMarginData->mPadding.mBottom = zero;
    aData->mMarginData->mPadding.mLeft = zero;
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSFirstLetterRule::MapRuleInfoInto(nsRuleData* aData)
{
  








  CommonMapRuleInfoInto(aData);

  
  
  

  return NS_OK;
}



class HTMLCSSStyleSheetImpl : public nsIHTMLCSSStyleSheet,
                              public nsIStyleRuleProcessor {
public:
  HTMLCSSStyleSheetImpl();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD Init(nsIURI* aURL, nsIDocument* aDocument);
  NS_IMETHOD Reset(nsIURI* aURL);
  NS_IMETHOD GetSheetURI(nsIURI** aSheetURL) const;
  NS_IMETHOD GetBaseURI(nsIURI** aBaseURL) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD_(PRBool) UseForMedium(nsPresContext* aPresContext) const;
  NS_IMETHOD_(PRBool) HasRules() const;

  NS_IMETHOD GetApplicable(PRBool& aApplicable) const;
  
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  NS_IMETHOD GetComplete(PRBool& aComplete) const;
  NS_IMETHOD SetComplete();

  
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocument);

  
  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData);

  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData);

  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult);

  NS_IMETHOD HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                        nsReStyleHint* aResult);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

private: 
  
  HTMLCSSStyleSheetImpl(const HTMLCSSStyleSheetImpl& aCopy); 
  HTMLCSSStyleSheetImpl& operator=(const HTMLCSSStyleSheetImpl& aCopy); 

protected:
  virtual ~HTMLCSSStyleSheetImpl();

protected:
  nsIURI*         mURL;
  nsIDocument*    mDocument;

  CSSFirstLineRule* mFirstLineRule;
  CSSFirstLetterRule* mFirstLetterRule;
};


HTMLCSSStyleSheetImpl::HTMLCSSStyleSheetImpl()
  : nsIHTMLCSSStyleSheet(),
    mRefCnt(0),
    mURL(nsnull),
    mDocument(nsnull),
    mFirstLineRule(nsnull),
    mFirstLetterRule(nsnull)
{
}

HTMLCSSStyleSheetImpl::~HTMLCSSStyleSheetImpl()
{
  NS_RELEASE(mURL);

  NS_IF_RELEASE(mFirstLineRule);
  NS_IF_RELEASE(mFirstLetterRule);
}

NS_IMPL_ISUPPORTS3(HTMLCSSStyleSheetImpl,
                   nsIHTMLCSSStyleSheet,
                   nsIStyleSheet,
                   nsIStyleRuleProcessor)

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(ElementRuleProcessorData* aData)
{
  nsIContent* content = aData->mContent;
  
  if (content) {
    
    nsICSSStyleRule* rule = content->GetInlineStyleRule();
    if (rule)
      aData->mRuleWalker->Forward(rule);
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::RulesMatching(PseudoRuleProcessorData* aData)
{
  
  
  
  
  if (aData->mRuleWalker->AtRoot())
    return NS_OK;

  nsIAtom* pseudoTag = aData->mPseudoTag;
  if (pseudoTag == nsCSSPseudoElements::firstLine) {
    if (!mFirstLineRule) {
      mFirstLineRule = new CSSFirstLineRule();
      if (!mFirstLineRule)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(mFirstLineRule);
    }
    aData->mRuleWalker->Forward(mFirstLineRule);
  }
  else if (pseudoTag == nsCSSPseudoElements::firstLetter) {
    if (!mFirstLetterRule) {
      mFirstLetterRule = new CSSFirstLetterRule();
      if (!mFirstLetterRule)
        return NS_ERROR_OUT_OF_MEMORY;
      NS_ADDREF(mFirstLetterRule);
    }
    aData->mRuleWalker->Forward(mFirstLetterRule);
  } 
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::Init(nsIURI* aURL, nsIDocument* aDocument)
{
  NS_PRECONDITION(aURL && aDocument, "null ptr");
  if (! aURL || ! aDocument)
    return NS_ERROR_NULL_POINTER;

  if (mURL || mDocument)
    return NS_ERROR_ALREADY_INITIALIZED;

  mDocument = aDocument; 
  mURL = aURL;
  NS_ADDREF(mURL);
  return NS_OK;
}


NS_IMETHODIMP
HTMLCSSStyleSheetImpl::HasStateDependentStyle(StateRuleProcessorData* aData,
                                              nsReStyleHint* aResult)
{
  *aResult = nsReStyleHint(0);
  return NS_OK;
}


NS_IMETHODIMP
HTMLCSSStyleSheetImpl::HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                                  nsReStyleHint* aResult)
{
  *aResult = nsReStyleHint(0);
  return NS_OK;
}



NS_IMETHODIMP 
HTMLCSSStyleSheetImpl::Reset(nsIURI* aURL)
{
  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_ADDREF(mURL);

  NS_IF_RELEASE(mFirstLineRule);
  NS_IF_RELEASE(mFirstLetterRule);
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetSheetURI(nsIURI** aSheetURL) const
{
  NS_IF_ADDREF(mURL);
  *aSheetURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetBaseURI(nsIURI** aBaseURL) const
{
  NS_IF_ADDREF(mURL);
  *aBaseURL = mURL;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetTitle(nsString& aTitle) const
{
  aTitle.AssignLiteral("Internal HTML/CSS Style Sheet");
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetType(nsString& aType) const
{
  aType.AssignLiteral("text/html");
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
HTMLCSSStyleSheetImpl::UseForMedium(nsPresContext* aPresContext) const
{
  return PR_TRUE; 
}

NS_IMETHODIMP_(PRBool)
HTMLCSSStyleSheetImpl::HasRules() const
{
  return PR_TRUE;  
                   
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetApplicable(PRBool& aApplicable) const
{
  aApplicable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetEnabled(PRBool aEnabled)
{ 
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetComplete(PRBool& aComplete) const
{
  aComplete = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetComplete()
{
  return NS_OK;
}


NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetParentSheet(nsIStyleSheet*& aParent) const
{
  aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::GetOwningDocument(nsIDocument*& aDocument) const
{
  NS_IF_ADDREF(mDocument);
  aDocument = mDocument;
  return NS_OK;
}

NS_IMETHODIMP
HTMLCSSStyleSheetImpl::SetOwningDocument(nsIDocument* aDocument)
{
  mDocument = aDocument;
  return NS_OK;
}

#ifdef DEBUG
void HTMLCSSStyleSheetImpl::List(FILE* out, PRInt32 aIndent) const
{
  
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("HTML CSS Style Sheet: ", out);
  nsCAutoString urlSpec;
  mURL->GetSpec(urlSpec);
  if (!urlSpec.IsEmpty()) {
    fputs(urlSpec.get(), out);
  }
  fputs("\n", out);
}
#endif


nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult,
                        nsIURI* aURL, nsIDocument* aDocument)
{
  nsresult rv;
  nsIHTMLCSSStyleSheet* sheet;
  if (NS_FAILED(rv = NS_NewHTMLCSSStyleSheet(&sheet)))
    return rv;

  if (NS_FAILED(rv = sheet->Init(aURL, aDocument))) {
    NS_RELEASE(sheet);
    return rv;
  }

  *aInstancePtrResult = sheet;
  return NS_OK;
}

nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  HTMLCSSStyleSheetImpl*  it = new HTMLCSSStyleSheetImpl();

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(it);
  *aInstancePtrResult = it;
  return NS_OK;
}
