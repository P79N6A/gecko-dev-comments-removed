




#ifndef nsColorPickerProxy_h
#define nsColorPickerProxy_h

#include "nsIColorPicker.h"

#include "mozilla/dom/PColorPickerChild.h"

class nsColorPickerProxy final : public nsIColorPicker,
                                 public mozilla::dom::PColorPickerChild
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOLORPICKER

  nsColorPickerProxy() {}

  virtual bool RecvUpdate(const nsString& aColor) override;
  virtual bool Recv__delete__(const nsString& aColor) override;

private:
  ~nsColorPickerProxy() {}

  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
  nsString mTitle;
  nsString mInitialColor;
};

#endif 
