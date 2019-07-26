





#ifndef nsColorPicker_h__
#define nsColorPicker_h__

#include <windows.h>
#include <commdlg.h>

#include "nsCOMPtr.h"
#include "nsIColorPicker.h"
#include "nsString.h"
#include "nsThreadUtils.h"

class nsIWidget;

class AsyncColorChooser :
  public nsRunnable
{
public:
  AsyncColorChooser(DWORD aInitialColor, nsIWidget* aParentWidget, nsIColorPickerShownCallback* aCallback);
  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  DWORD mInitialColor;
  nsCOMPtr<nsIWidget> mParentWidget;
  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
  nsString mColor;
};

class nsColorPicker :
  public nsIColorPicker
{
public:
  nsColorPicker();
  virtual ~nsColorPicker();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDOMWindow* parent, const nsAString& title, const nsAString& aInitialColor);
  NS_IMETHOD Open(nsIColorPickerShownCallback* aCallback);

protected:
  DWORD mInitialColor;
  nsCOMPtr<nsIWidget> mParentWidget;
};

#endif 
