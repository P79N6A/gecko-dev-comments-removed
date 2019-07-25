







































#ifndef nsWinUtils_h_
#define nsWinUtils_h_

#include "Accessible2.h"

#include "nsIArray.h"
#include "nsIDocument.h"

const LPCWSTR kClassNameRoot = L"MozillaUIWindowClass";
const LPCWSTR kClassNameTabContent = L"MozillaContentWindowClass";

const LPCWSTR kJAWSModuleHandle = L"jhook";
const LPCWSTR kWEModuleHandle = L"gwm32inc";
const LPCWSTR kDolphnModuleHandle = L"dolwinhk";

class nsWinUtils
{
public:
  



  static HRESULT ConvertToIA2Array(nsIArray *aCollection,
                                   IUnknown ***aAccessibles, long *aCount);

  


  static void RegisterNativeWindow(LPCWSTR aWindowClass);

  


  static HWND CreateNativeWindow(LPCWSTR aWindowClass, HWND aParentWnd);

  


  static bool IsWindowEmulationEnabled();

  


  static bool IsTabDocument(nsIDocument* aDocumentNode);
};

#endif

