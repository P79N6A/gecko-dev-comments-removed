



#ifndef nsFlyOwnDialog_h___
#define nsFlyOwnDialog_h___

nsresult NativeShowPrintDialog(HWND                aHWnd,
                               nsIWebBrowserPrint* aWebBrowserPrint,
                               nsIPrintSettings*   aPrintSettings);

HGLOBAL CreateGlobalDevModeAndInit(const nsXPIDLString& aPrintName, nsIPrintSettings* aPS);

#endif 
