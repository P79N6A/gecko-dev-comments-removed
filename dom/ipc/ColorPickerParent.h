





#ifndef mozilla_dom_ColorPickerParent_h
#define mozilla_dom_ColorPickerParent_h

#include "mozilla/dom/PColorPickerParent.h"
#include "nsIColorPicker.h"

namespace mozilla {
namespace dom {

class ColorPickerParent : public PColorPickerParent
{
 public:
  ColorPickerParent(const nsString& aTitle,
                    const nsString& aInitialColor)
  : mTitle(aTitle)
  , mInitialColor(aInitialColor)
  {}

  virtual bool RecvOpen() override;
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  class ColorPickerShownCallback final
    : public nsIColorPickerShownCallback
  {
  public:
    explicit ColorPickerShownCallback(ColorPickerParent* aColorPickerParnet)
      : mColorPickerParent(aColorPickerParnet)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSICOLORPICKERSHOWNCALLBACK

    void Destroy();

  private:
    ~ColorPickerShownCallback() {}
    ColorPickerParent* mColorPickerParent;
  };

 private:
  virtual ~ColorPickerParent() {}

  bool CreateColorPicker();

  nsRefPtr<ColorPickerShownCallback> mCallback;
  nsCOMPtr<nsIColorPicker> mPicker;

  nsString mTitle;
  nsString mInitialColor;
};

} 
} 

#endif 
