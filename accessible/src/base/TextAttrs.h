




#ifndef nsTextAttrs_h_
#define nsTextAttrs_h_

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIPersistentProperties2.h"
#include "nsStyleConsts.h"

class HyperTextAccessible;

namespace mozilla {
namespace a11y {








class TextAttrsMgr
{
public:
  


  TextAttrsMgr(HyperTextAccessible* aHyperTextAcc) :
    mOffsetAcc(nsnull),  mHyperTextAcc(aHyperTextAcc),
    mOffsetAccIdx(-1), mIncludeDefAttrs(true) { }

  











  TextAttrsMgr(HyperTextAccessible* aHyperTextAcc,
               bool aIncludeDefAttrs,
               Accessible* aOffsetAcc,
               PRInt32 aOffsetAccIdx) :
    mOffsetAcc(aOffsetAcc), mHyperTextAcc(aHyperTextAcc),
    mOffsetAccIdx(aOffsetAccIdx), mIncludeDefAttrs(aIncludeDefAttrs) { }

  










  void GetAttributes(nsIPersistentProperties* aAttributes,
                     PRInt32* aStartHTOffset = nsnull,
                     PRInt32* aEndHTOffset = nsnull);

protected:
  









  class TextAttr;
  void GetRange(TextAttr* aAttrArray[], PRUint32 aAttrArrayLen,
                PRInt32* aStartHTOffset, PRInt32* aEndHTOffset);

private:
  Accessible* mOffsetAcc;
  HyperTextAccessible* mHyperTextAcc;
  PRInt32 mOffsetAccIdx;
  bool mIncludeDefAttrs;

protected:

  


  class TextAttr
  {
  public:
    






    virtual void Expose(nsIPersistentProperties* aAttributes,
                        bool aIncludeDefAttrValue) = 0;

    



    virtual bool Equal(nsIContent* aElm) = 0;
  };


  


  template<class T>
  class TTextAttr : public TextAttr
  {
  public:
    TTextAttr(bool aGetRootValue) : mGetRootValue(aGetRootValue) {}

    
    virtual void Expose(nsIPersistentProperties* aAttributes,
                        bool aIncludeDefAttrValue)
    {
      if (mGetRootValue) {
        if (mIsRootDefined)
          ExposeValue(aAttributes, mRootNativeValue);
        return;
      }

      if (mIsDefined) {
        if (aIncludeDefAttrValue || mRootNativeValue != mNativeValue)
          ExposeValue(aAttributes, mNativeValue);
        return;
      }

      if (aIncludeDefAttrValue && mIsRootDefined)
        ExposeValue(aAttributes, mRootNativeValue);
    }

    virtual bool Equal(nsIContent* aElm)
    {
      T nativeValue;
      bool isDefined = GetValueFor(aElm, &nativeValue);

      if (!mIsDefined && !isDefined)
        return true;

      if (mIsDefined && isDefined)
        return nativeValue == mNativeValue;

      if (mIsDefined)
        return mNativeValue == mRootNativeValue;

      return nativeValue == mRootNativeValue;
    }

  protected:

    
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const T& aValue) = 0;

    
    virtual bool GetValueFor(nsIContent* aElm, T* aValue) = 0;

    
    bool mGetRootValue;

    
    
    
    T mNativeValue;
    bool mIsDefined;

    
    
    T mRootNativeValue;
    bool mIsRootDefined;
  };


  


  class LangTextAttr : public TTextAttr<nsString>
  {
  public:
    LangTextAttr(HyperTextAccessible* aRoot, nsIContent* aRootElm,
                 nsIContent* aElm);
    virtual ~LangTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, nsString* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nsString& aValue);

  private:
    bool GetLang(nsIContent* aElm, nsAString& aLang);
    nsCOMPtr<nsIContent> mRootContent;
  };


  


  class BGColorTextAttr : public TTextAttr<nscolor>
  {
  public:
    BGColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~BGColorTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, nscolor* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscolor& aValue);

  private:
    bool GetColor(nsIFrame* aFrame, nscolor* aColor);
    nsIFrame* mRootFrame;
  };


  


  class ColorTextAttr : public TTextAttr<nscolor>
  {
  public:
    ColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~ColorTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, nscolor* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscolor& aValue);
  };


  


  class FontFamilyTextAttr : public TTextAttr<nsString>
  {
  public:
    FontFamilyTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontFamilyTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, nsString* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nsString& aValue);

  private:

    bool GetFontFamily(nsIFrame* aFrame, nsString& aFamily);
  };


  


  class FontSizeTextAttr : public TTextAttr<nscoord>
  {
  public:
    FontSizeTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontSizeTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, nscoord* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscoord& aValue);

  private:
    nsDeviceContext* mDC;
  };


  


  class FontStyleTextAttr : public TTextAttr<nscoord>
  {
  public:
    FontStyleTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontStyleTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aContent, nscoord* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscoord& aValue);
  };


  


  class FontWeightTextAttr : public TTextAttr<PRInt32>
  {
  public:
    FontWeightTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontWeightTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, PRInt32* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const PRInt32& aValue);

  private:
    PRInt32 GetFontWeight(nsIFrame* aFrame);
  };


  





  class TextDecorValue
  {
  public:
    TextDecorValue() { }
    TextDecorValue(nsIFrame* aFrame);

    nscolor Color() const { return mColor; }
    PRUint8 Style() const { return mStyle; }

    bool IsDefined() const
      { return IsUnderline() || IsLineThrough(); }
    bool IsUnderline() const
      { return mLine & NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE; }
    bool IsLineThrough() const
      { return mLine & NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH; }

    bool operator ==(const TextDecorValue& aValue)
    {
      return mColor == aValue.mColor && mLine == aValue.mLine &&
        mStyle == aValue.mStyle;
    }
    bool operator !=(const TextDecorValue& aValue)
      { return !(*this == aValue); }

  private:
    nscolor mColor;
    PRUint8 mLine;
    PRUint8 mStyle;
  };

  class TextDecorTextAttr : public TTextAttr<TextDecorValue>
  {
  public:
    TextDecorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~TextDecorTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, TextDecorValue* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const TextDecorValue& aValue);
  };

  



  enum TextPosValue {
    eTextPosNone = 0,
    eTextPosBaseline,
    eTextPosSub,
    eTextPosSuper
  };

  class TextPosTextAttr : public TTextAttr<TextPosValue>
  {
  public:
    TextPosTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~TextPosTextAttr() { }

  protected:

    
    virtual bool GetValueFor(nsIContent* aElm, TextPosValue* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const TextPosValue& aValue);

  private:
    TextPosValue GetTextPosValue(nsIFrame* aFrame) const;
  };

}; 

} 
} 

#endif
