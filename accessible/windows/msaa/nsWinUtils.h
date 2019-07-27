






#ifndef nsWinUtils_h_
#define nsWinUtils_h_

#include <windows.h>

#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCOMPtr.h"
#include "nsRefPtrHashtable.h"

class nsIContent;

namespace mozilla {
namespace a11y {

class DocAccessible;

const LPCWSTR kClassNameRoot = L"MozillaUIWindowClass";
const LPCWSTR kClassNameTabContent = L"MozillaContentWindowClass";

class nsWinUtils
{
public:
  





  static already_AddRefed<nsIDOMCSSStyleDeclaration>
    GetComputedStyleDeclaration(nsIContent* aContent);

  


  static bool MaybeStartWindowEmulation();

  


  static void ShutdownWindowEmulation();

  


  static bool IsWindowEmulationStarted();

  


  static void RegisterNativeWindow(LPCWSTR aWindowClass);

  


  static HWND CreateNativeWindow(LPCWSTR aWindowClass, HWND aParentWnd,
                                 int aX, int aY, int aWidth, int aHeight,
                                 bool aIsActive);

  


  static void ShowNativeWindow(HWND aWnd);

  


  static void HideNativeWindow(HWND aWnd);

  



  static nsRefPtrHashtable<nsPtrHashKey<void>, DocAccessible>* sHWNDCache;
};

} 
} 

#endif
