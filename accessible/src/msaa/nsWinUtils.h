







































#ifndef nsWinUtils_h_
#define nsWinUtils_h_

#include "Accessible2.h"

#include "nsIArray.h"
#include "nsIDocument.h"

const LPCWSTR kClassNameRoot = L"MozillaUIWindowClass";
const LPCWSTR kClassNameTabContent = L"MozillaContentWindowClass";

const LPCWSTR kJAWSModuleHandle = L"jhook";
const LPCWSTR kWEModuleHandle = L"gwm32inc";
const LPCWSTR kDolphinModuleHandle = L"dolwinhk";

class nsWinUtils
{
public:
  



  static HRESULT ConvertToIA2Array(nsIArray *aCollection,
                                   IUnknown ***aAccessibles, long *aCount);

  


  static void RegisterNativeWindow(LPCWSTR aWindowClass);

  


  static HWND CreateNativeWindow(LPCWSTR aWindowClass, HWND aParentWnd,
                                 int aX, int aY, int aWidth, int aHeight,
                                 bool aIsActive);

  


  static void ShowNativeWindow(HWND aWnd);

  


  static void HideNativeWindow(HWND aWnd);

  


  static bool IsWindowEmulationEnabled(LPCWSTR kModuleHandle = 0);

  


  static bool IsTabDocument(nsIDocument* aDocumentNode);
};

#endif

