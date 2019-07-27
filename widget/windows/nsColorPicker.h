





#ifndef nsColorPicker_h__
#define nsColorPicker_h__

#include <windows.h>
#include <commdlg.h>

#include "nsCOMPtr.h"
#include "nsIColorPicker.h"
#include "nsThreadUtils.h"

class nsIWidget;

class AsyncColorChooser :
  public nsRunnable
{
public:
  AsyncColorChooser(COLORREF aInitialColor,
                    nsIWidget* aParentWidget,
                    nsIColorPickerShownCallback* aCallback);
  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  void Update(COLORREF aColor);

  static UINT_PTR CALLBACK HookProc(HWND aDialog, UINT aMsg,
                                    WPARAM aWParam, LPARAM aLParam);

  COLORREF mInitialColor;
  COLORREF mColor;
  nsCOMPtr<nsIWidget> mParentWidget;
  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
};

class nsColorPicker :
  public nsIColorPicker
{
  virtual ~nsColorPicker();

public:
  nsColorPicker();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDOMWindow* parent, const nsAString& title,
                  const nsAString& aInitialColor);
  NS_IMETHOD Open(nsIColorPickerShownCallback* aCallback);

private:
  COLORREF mInitialColor;
  nsCOMPtr<nsIWidget> mParentWidget;
};

#endif 
