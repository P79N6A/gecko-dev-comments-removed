





#ifndef nsColorPicker_h_
#define nsColorPicker_h_

#include "nsIColorPicker.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIColorPickerShownCallback;
class nsIDOMWindow;
@class NSColorPanelWrapper;
@class NSColor;

class nsColorPicker MOZ_FINAL : public nsIColorPicker
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDOMWindow* aParent, const nsAString& aTitle,
                  const nsAString& aInitialColor);
  NS_IMETHOD Open(nsIColorPickerShownCallback* aCallback);

  
  void Update(NSColor* aColor);
  
  
  void DoneWithRetarget();
  
  
  void Done();

private:
  ~nsColorPicker();

  static NSColor* GetNSColorFromHexString(const nsAString& aColor);
  static void GetHexStringFromNSColor(NSColor* aColor, nsAString& aResult);

  static NSColorPanelWrapper* sColorPanelWrapper;

  nsString             mTitle;
  nsString             mColor;
  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
};

#endif 
