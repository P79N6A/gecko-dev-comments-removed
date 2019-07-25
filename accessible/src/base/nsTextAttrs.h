





































#ifndef nsTextAttrs_h_
#define nsTextAttrs_h_

class nsHyperTextAccessible;

#include "nsAccessibilityAtoms.h"

#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

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
                 PRBool aIncludeDefAttrs = PR_TRUE,
                 nsAccessible *aOffsetAcc = nsnull,
                 PRInt32 aOffsetAccIdx = -1);

  










  nsresult GetAttributes(nsIPersistentProperties *aAttributes,
                         PRInt32 *aStartHTOffset = nsnull,
                         PRInt32 *aEndHTOffset = nsnull);

protected:

  








   nsresult GetRange(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                     PRInt32 *aStartHTOffset, PRInt32 *aEndHTOffset);

private:
  nsRefPtr<nsHyperTextAccessible> mHyperTextAcc;

  PRBool mIncludeDefAttrs;

  nsRefPtr<nsAccessible> mOffsetAcc;
  PRInt32 mOffsetAccIdx;
};








class nsITextAttr
{
public:
  


  virtual nsIAtom* GetName() = 0;

  










  virtual PRBool GetValue(nsAString& aValue, PRBool aIncludeDefAttrValue) = 0;

  



  virtual PRBool Equal(nsIContent *aContent) = 0;
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

  virtual PRBool Equal(nsIContent *aContent)
  {
    T nativeValue;
    PRBool isDefined = GetValueFor(aContent, &nativeValue);

    if (!mIsDefined && !isDefined)
      return PR_TRUE;

    if (mIsDefined && isDefined)
      return nativeValue == mNativeValue;

    if (mIsDefined)
      return mNativeValue == mRootNativeValue;

    return nativeValue == mRootNativeValue;
  }

protected:

  
  virtual PRBool GetValueFor(nsIContent *aContent, T *aValue) = 0;

  
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
  nsLangTextAttr(nsHyperTextAccessible *aRootAcc, nsIContent *aRootContent,
                 nsIContent *aContent);

  
  virtual nsIAtom *GetName() { return nsAccessibilityAtoms::language; }

protected:

  
  virtual PRBool GetValueFor(nsIContent *aElm, nsAutoString *aValue);
  virtual void Format(const nsAutoString& aValue, nsAString& aFormattedValue);

private:
  PRBool GetLang(nsIContent *aContent, nsAString& aLang);
  nsCOMPtr<nsIContent> mRootContent;
};






class nsCSSTextAttr : public nsTextAttr<nsAutoString>
{
public:
  nsCSSTextAttr(PRUint32 aIndex, nsIContent *aRootContent,
                nsIContent *aContent);

  
  virtual nsIAtom *GetName();

protected:

  
  virtual PRBool GetValueFor(nsIContent *aContent, nsAutoString *aValue);
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
  
  virtual PRBool GetValueFor(nsIContent *aContent, nscolor *aValue);
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

  
  virtual PRBool GetValueFor(nsIContent *aContent, nscoord *aValue);
  virtual void Format(const nscoord& aValue, nsAString& aFormattedValue);

private:

  





   nscoord GetFontSize(nsIFrame *aFrame);

  nsDeviceContext *mDC;
};






class nsFontWeightTextAttr : public nsTextAttr<PRInt32>
{
public:
  nsFontWeightTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() { return nsAccessibilityAtoms::fontWeight; }

protected:

  
  virtual PRBool GetValueFor(nsIContent *aElm, PRInt32 *aValue);
  virtual void Format(const PRInt32& aValue, nsAString& aFormattedValue);

private:

  





  PRInt32 GetFontWeight(nsIFrame *aFrame);
};

#endif
