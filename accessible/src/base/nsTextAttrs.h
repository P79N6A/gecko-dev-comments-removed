





































#ifndef nsTextAttrs_h_
#define nsTextAttrs_h_

class nsHyperTextAccessible;

#include "nsAccessibilityAtoms.h"

#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPersistentProperties2.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTPtrArray.h"

class nsITextAttr;









class nsTextAttrsMgr
{
public:
  













  nsTextAttrsMgr(nsHyperTextAccessible *aHyperTextAcc,
                 nsIDOMNode *aHyperTextNode,
                 PRBool aIncludeDefAttrs = PR_TRUE,
                 nsIDOMNode *oOffsetNode = nsnull);

  










  nsresult GetAttributes(nsIPersistentProperties *aAttributes,
                         PRInt32 *aStartHTOffset = nsnull,
                         PRInt32 *aEndHTOffset = nsnull);

protected:

  








   nsresult GetRange(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                     PRInt32 *aStartHTOffset, PRInt32 *aEndHTOffset);

  









   PRBool FindEndOffsetInSubtree(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                                 nsIDOMNode *aCurrNode, PRInt32 *aHTOffset);

  











   PRBool FindStartOffsetInSubtree(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                                   nsIDOMNode *aCurrNode, nsIDOMNode *aPrevNode,
                                   PRInt32 *aHTOffset);

private:
  nsRefPtr<nsHyperTextAccessible> mHyperTextAcc;
  nsCOMPtr<nsIDOMNode> mHyperTextNode;

  PRBool mIncludeDefAttrs;
  nsCOMPtr<nsIDOMNode> mOffsetNode;
};








class nsITextAttr
{
public:
  


  virtual nsIAtom* GetName() = 0;

  










  virtual PRBool GetValue(nsAString& aValue, PRBool aIncludeDefAttrValue) = 0;

  



  virtual PRBool Equal(nsIDOMElement *aElm) = 0;
};





template<class T>
class nsTextAttr : public nsITextAttr
{
public:
  nsTextAttr(PRBool aGetRootValue) : mGetRootValue(aGetRootValue) {}

  
  virtual PRBool GetValue(nsAString& aValue, PRBool aIncludeDefAttrValue)
  {
    if (mGetRootValue) {
      Format(mRootNativeValue, aValue);
      return mIsRootDefined;
    }

    PRBool isDefined = mIsDefined;
    T* nativeValue = &mNativeValue;

    if (!isDefined) {
      if (aIncludeDefAttrValue) {
        isDefined = mIsRootDefined;
        nativeValue = &mRootNativeValue;
      }
    } else if (!aIncludeDefAttrValue) {
      isDefined = mRootNativeValue != mNativeValue;
    }

    if (!isDefined)
      return PR_FALSE;

    Format(*nativeValue, aValue);
    return PR_TRUE;
  }

  virtual PRBool Equal(nsIDOMElement *aElm)
  {
    T nativeValue;
    PRBool isDefined = GetValueFor(aElm, &nativeValue);

    if (!mIsDefined && !isDefined)
      return PR_TRUE;

    if (mIsDefined && isDefined)
      return nativeValue == mNativeValue;

    if (mIsDefined)
      return mNativeValue == mRootNativeValue;

    return nativeValue == mRootNativeValue;
  }

protected:

  
  virtual PRBool GetValueFor(nsIDOMElement *aElm, T *aValue) = 0;

  
  virtual void Format(const T& aValue, nsAString& aFormattedValue) = 0;

  
  PRBool mGetRootValue;

  
  
  
  T mNativeValue;
  PRBool mIsDefined;

  
  
  T mRootNativeValue;
  PRBool mIsRootDefined;
};






class nsLangTextAttr : public nsTextAttr<nsAutoString>
{
public:
  nsLangTextAttr(nsHyperTextAccessible *aRootAcc, nsIDOMNode *aRootNode,
                 nsIDOMNode *aNode);

  
  virtual nsIAtom *GetName() { return nsAccessibilityAtoms::language; }

protected:

  
  virtual PRBool GetValueFor(nsIDOMElement *aElm, nsAutoString *aValue);
  virtual void Format(const nsAutoString& aValue, nsAString& aFormattedValue);

private:
  PRBool GetLang(nsIContent *aContent, nsAString& aLang);
  nsCOMPtr<nsIContent> mRootContent;
};






class nsCSSTextAttr : public nsTextAttr<nsAutoString>
{
public:
  nsCSSTextAttr(PRUint32 aIndex, nsIDOMElement *aRootElm, nsIDOMElement *aElm);

  
  virtual nsIAtom *GetName();

protected:

  
  virtual PRBool GetValueFor(nsIDOMElement *aElm, nsAutoString *aValue);
  virtual void Format(const nsAutoString& aValue, nsAString& aFormattedValue);

private:
  PRInt32 mIndex;
};






class nsBGColorTextAttr : public nsTextAttr<nscolor>
{
public:
  nsBGColorTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() { return nsAccessibilityAtoms::backgroundColor; }

protected:
  
  virtual PRBool GetValueFor(nsIDOMElement *aElm, nscolor *aValue);
  virtual void Format(const nscolor& aValue, nsAString& aFormattedValue);

private:
  PRBool GetColor(nsIFrame *aFrame, nscolor *aColor);
  nsIFrame *mRootFrame;
};






class nsFontSizeTextAttr : public nsTextAttr<nscoord>
{
public:
  nsFontSizeTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() { return nsAccessibilityAtoms::fontSize; }

protected:

  
  virtual PRBool GetValueFor(nsIDOMElement *aElm, nscoord *aValue);
  virtual void Format(const nscoord& aValue, nsAString& aFormattedValue);

private:

  





   nscoord GetFontSize(nsIFrame *aFrame);

  nsIDeviceContext *mDC;
};

#endif
