




#ifndef nsTextAttrs_h_
#define nsTextAttrs_h_

#include "nsCOMPtr.h"
#include "nsColor.h"
#include "nsStyleConsts.h"

class nsIFrame;
class nsIPersistentProperties;
class nsIContent;
class nsDeviceContext;

namespace mozilla {
namespace a11y {

class Accessible;
class HyperTextAccessible;








class TextAttrsMgr
{
public:
  


  explicit TextAttrsMgr(HyperTextAccessible* aHyperTextAcc) :
    mOffsetAcc(nullptr),  mHyperTextAcc(aHyperTextAcc),
    mOffsetAccIdx(-1), mIncludeDefAttrs(true) { }

  











  TextAttrsMgr(HyperTextAccessible* aHyperTextAcc,
               bool aIncludeDefAttrs,
               Accessible* aOffsetAcc,
               int32_t aOffsetAccIdx) :
    mOffsetAcc(aOffsetAcc), mHyperTextAcc(aHyperTextAcc),
    mOffsetAccIdx(aOffsetAccIdx), mIncludeDefAttrs(aIncludeDefAttrs) { }

  










  void GetAttributes(nsIPersistentProperties* aAttributes,
                     uint32_t* aStartHTOffset = nullptr,
                     uint32_t* aEndHTOffset = nullptr);

protected:
  









  class TextAttr;
  void GetRange(TextAttr* aAttrArray[], uint32_t aAttrArrayLen,
                uint32_t* aStartOffset, uint32_t* aEndOffset);

private:
  Accessible* mOffsetAcc;
  HyperTextAccessible* mHyperTextAcc;
  int32_t mOffsetAccIdx;
  bool mIncludeDefAttrs;

protected:

  


  class TextAttr
  {
  public:
    






    virtual void Expose(nsIPersistentProperties* aAttributes,
                        bool aIncludeDefAttrValue) = 0;

    



    virtual bool Equal(Accessible* aAccessible) = 0;
  };


  


  template<class T>
  class TTextAttr : public TextAttr
  {
  public:
    explicit TTextAttr(bool aGetRootValue) : mGetRootValue(aGetRootValue) {}

    
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

    virtual bool Equal(Accessible* aAccessible)
    {
      T nativeValue;
      bool isDefined = GetValueFor(aAccessible, &nativeValue);

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

    
    virtual bool GetValueFor(Accessible* aAccessible, T* aValue) = 0;

    
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
    virtual ~LangTextAttr();

  protected:

    
    virtual bool GetValueFor(Accessible* aAccessible, nsString* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nsString& aValue);

  private:
    nsCOMPtr<nsIContent> mRootContent;
  };


  





  class InvalidTextAttr : public TTextAttr<uint32_t>
  {
  public:
    InvalidTextAttr(nsIContent* aRootElm, nsIContent* aElm);
    virtual ~InvalidTextAttr() { };

  protected:

    enum {
      eFalse,
      eGrammar,
      eSpelling,
      eTrue
    };

    
    virtual bool GetValueFor(Accessible* aAccessible, uint32_t* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const uint32_t& aValue);

  private:
    bool GetValue(nsIContent* aElm, uint32_t* aValue);
    nsIContent* mRootElm;
  };


  


  class BGColorTextAttr : public TTextAttr<nscolor>
  {
  public:
    BGColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~BGColorTextAttr() { }

  protected:

    
    virtual bool GetValueFor(Accessible* aAccessible, nscolor* aValue);
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

    
    virtual bool GetValueFor(Accessible* aAccessible, nscolor* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscolor& aValue);
  };


  


  class FontFamilyTextAttr : public TTextAttr<nsString>
  {
  public:
    FontFamilyTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontFamilyTextAttr() { }

  protected:

    
    virtual bool GetValueFor(Accessible* aAccessible, nsString* aValue);
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

    
    virtual bool GetValueFor(Accessible* aAccessible, nscoord* aValue);
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

    
    virtual bool GetValueFor(Accessible* aContent, nscoord* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const nscoord& aValue);
  };


  


  class FontWeightTextAttr : public TTextAttr<int32_t>
  {
  public:
    FontWeightTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~FontWeightTextAttr() { }

  protected:

    
    virtual bool GetValueFor(Accessible* aAccessible, int32_t* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const int32_t& aValue);

  private:
    int32_t GetFontWeight(nsIFrame* aFrame);
  };

  


  class AutoGeneratedTextAttr : public TTextAttr<bool>
  {
  public:
    AutoGeneratedTextAttr(HyperTextAccessible* aHyperTextAcc,
                          Accessible* aAccessible);
    virtual ~AutoGeneratedTextAttr() { }

  protected:
    
    virtual bool GetValueFor(Accessible* aAccessible, bool* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const bool& aValue);
  };


  





  class TextDecorValue
  {
  public:
    TextDecorValue() { }
    explicit TextDecorValue(nsIFrame* aFrame);

    nscolor Color() const { return mColor; }
    uint8_t Style() const { return mStyle; }

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
    uint8_t mLine;
    uint8_t mStyle;
  };

  class TextDecorTextAttr : public TTextAttr<TextDecorValue>
  {
  public:
    TextDecorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame);
    virtual ~TextDecorTextAttr() { }

  protected:

    
    virtual bool GetValueFor(Accessible* aAccessible, TextDecorValue* aValue);
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

    
    virtual bool GetValueFor(Accessible* aAccessible, TextPosValue* aValue);
    virtual void ExposeValue(nsIPersistentProperties* aAttributes,
                             const TextPosValue& aValue);

  private:
    TextPosValue GetTextPosValue(nsIFrame* aFrame) const;
  };

}; 

} 
} 

#endif
