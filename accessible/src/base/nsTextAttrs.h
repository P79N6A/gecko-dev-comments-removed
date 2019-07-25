





































#ifndef nsTextAttrs_h_
#define nsTextAttrs_h_

class nsHyperTextAccessible;


#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPersistentProperties2.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsITextAttr;









class nsTextAttrsMgr
{
public:
  













  nsTextAttrsMgr(nsHyperTextAccessible *aHyperTextAcc,
                 bool aIncludeDefAttrs = true,
                 nsAccessible *aOffsetAcc = nsnull,
                 PRInt32 aOffsetAccIdx = -1);

  










  nsresult GetAttributes(nsIPersistentProperties *aAttributes,
                         PRInt32 *aStartHTOffset = nsnull,
                         PRInt32 *aEndHTOffset = nsnull);

protected:

  








   nsresult GetRange(const nsTArray<nsITextAttr*>& aTextAttrArray,
                     PRInt32 *aStartHTOffset, PRInt32 *aEndHTOffset);

private:
  nsRefPtr<nsHyperTextAccessible> mHyperTextAcc;

  bool mIncludeDefAttrs;

  nsRefPtr<nsAccessible> mOffsetAcc;
  PRInt32 mOffsetAccIdx;
};








class nsITextAttr
{
public:
  


  virtual nsIAtom* GetName() const = 0;

  










  virtual bool GetValue(nsAString& aValue, bool aIncludeDefAttrValue) = 0;

  



  virtual bool Equal(nsIContent *aContent) = 0;
};





template<class T>
class nsTextAttr : public nsITextAttr
{
public:
  nsTextAttr(bool aGetRootValue) : mGetRootValue(aGetRootValue) {}

  
  virtual bool GetValue(nsAString& aValue, bool aIncludeDefAttrValue)
  {
    if (mGetRootValue) {
      Format(mRootNativeValue, aValue);
      return mIsRootDefined;
    }

    bool isDefined = mIsDefined;
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

  virtual bool Equal(nsIContent *aContent)
  {
    T nativeValue;
    bool isDefined = GetValueFor(aContent, &nativeValue);

    if (!mIsDefined && !isDefined)
      return PR_TRUE;

    if (mIsDefined && isDefined)
      return nativeValue == mNativeValue;

    if (mIsDefined)
      return mNativeValue == mRootNativeValue;

    return nativeValue == mRootNativeValue;
  }

protected:

  
  virtual bool GetValueFor(nsIContent *aContent, T *aValue) = 0;

  
  virtual void Format(const T& aValue, nsAString& aFormattedValue) = 0;

  
  bool mGetRootValue;

  
  
  
  T mNativeValue;
  bool mIsDefined;

  
  
  T mRootNativeValue;
  bool mIsRootDefined;
};






class nsLangTextAttr : public nsTextAttr<nsAutoString>
{
public:
  nsLangTextAttr(nsHyperTextAccessible *aRootAcc, nsIContent *aRootContent,
                 nsIContent *aContent);

  
  virtual nsIAtom *GetName() const { return nsGkAtoms::language; }

protected:

  
  virtual bool GetValueFor(nsIContent *aElm, nsAutoString *aValue);
  virtual void Format(const nsAutoString& aValue, nsAString& aFormattedValue);

private:
  bool GetLang(nsIContent *aContent, nsAString& aLang);
  nsCOMPtr<nsIContent> mRootContent;
};






class nsCSSTextAttr : public nsTextAttr<nsAutoString>
{
public:
  nsCSSTextAttr(PRUint32 aIndex, nsIContent *aRootContent,
                nsIContent *aContent);

  
  virtual nsIAtom *GetName() const;

protected:

  
  virtual bool GetValueFor(nsIContent *aContent, nsAutoString *aValue);
  virtual void Format(const nsAutoString& aValue, nsAString& aFormattedValue);

private:
  PRInt32 mIndex;
};






class nsBGColorTextAttr : public nsTextAttr<nscolor>
{
public:
  nsBGColorTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() const { return nsGkAtoms::backgroundColor; }

protected:
  
  virtual bool GetValueFor(nsIContent *aContent, nscolor *aValue);
  virtual void Format(const nscolor& aValue, nsAString& aFormattedValue);

private:
  bool GetColor(nsIFrame *aFrame, nscolor *aColor);
  nsIFrame *mRootFrame;
};






class nsFontSizeTextAttr : public nsTextAttr<nscoord>
{
public:
  nsFontSizeTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() const { return nsGkAtoms::font_size; }

protected:

  
  virtual bool GetValueFor(nsIContent *aContent, nscoord *aValue);
  virtual void Format(const nscoord& aValue, nsAString& aFormattedValue);

private:

  





   nscoord GetFontSize(nsIFrame *aFrame);

  nsDeviceContext *mDC;
};






class nsFontWeightTextAttr : public nsTextAttr<PRInt32>
{
public:
  nsFontWeightTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame);

  
  virtual nsIAtom *GetName() const { return nsGkAtoms::fontWeight; }

protected:

  
  virtual bool GetValueFor(nsIContent *aElm, PRInt32 *aValue);
  virtual void Format(const PRInt32& aValue, nsAString& aFormattedValue);

private:

  





  PRInt32 GetFontWeight(nsIFrame *aFrame);
};

#endif
