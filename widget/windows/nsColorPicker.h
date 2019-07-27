





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
  AsyncColorChooser(const nsAString& aInitialColor,
                    nsIWidget* aParentWidget,
                    nsIColorPickerShownCallback* aCallback);
  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  nsString mInitialColor;
  nsCOMPtr<nsIWidget> mParentWidget;
  nsCOMPtr<nsIColorPickerShownCallback> mCallback;
  nsString mColor;
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

protected:
  nsString mInitialColor;
  nsCOMPtr<nsIWidget> mParentWidget;
};

#endif 
