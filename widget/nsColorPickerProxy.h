




#ifndef nsColorPickerProxy_h
#define nsColorPickerProxy_h

#include "nsIColorPicker.h"

#include "mozilla/dom/PColorPickerChild.h"

class nsColorPickerProxy MOZ_FINAL : public nsIColorPicker,
                                     public mozilla::dom::PColorPickerChild
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOLORPICKER

  nsColorPickerProxy() {}

  virtual bool RecvUpdate(const nsString& aColor) MOZ_OVERRIDE;
  virtual bool Recv__delete__(const nsString& aColor) MOZ_OVERRIDE;

private:
  ~nsColorPickerProxy() {}

  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
  nsString mTitle;
  nsString mInitialColor;
};

#endif 
