








































#include "nsIDocShell.h"
#include "nsUrlWidget.h"
#include "nsPIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsCOMPtr.h"

#include <windows.h>






nsresult    
nsUrlWidget::Init()
{

   nsresult rv = NS_OK;

  return rv;
}

NS_IMETHODIMP
nsUrlWidget::SetURLToHiddenControl( char const *aURL, nsIDOMWindowInternal *parent )
{
	nsresult rv = NS_OK;
	HWND	hEdit=NULL; 
	HWND	hMainFrame=NULL;  
							

    static const LONG editControlID = 12345;

    nsCOMPtr<nsPIDOMWindow> win( do_QueryInterface(parent) );
    if (!win)
    {
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIBaseWindow> ppBaseWindow =
      do_QueryInterface(win->GetDocShell());

    if (ppBaseWindow)
    {
        nsCOMPtr<nsIWidget> ppWidget;
        ppBaseWindow->GetMainWidget(getter_AddRefs(ppWidget));
        hMainFrame = (HWND)ppWidget->GetNativeData(NS_NATIVE_WIDGET);
    }

	if (!hMainFrame)
	{
		return NS_ERROR_FAILURE;
	}

    
    hEdit = GetDlgItem( hMainFrame, 12345 );

    if ( !hEdit ) {
        ULONG visibility = 0;
        
        

        hEdit = ::CreateWindow("Edit",
                "",
                WS_CHILD | WS_BORDER | visibility,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                200,
                15,
                hMainFrame,
                (HMENU)editControlID,
                NULL,
                NULL);
    }

	
	if ((aURL != NULL) && (hEdit != NULL))
	{
        #ifdef DEBUG_URLWIDGET
        printf( "nsUrlWidget; window=0x%08X, url=[%s]\n", (int)hEdit, aURL );
        #endif
		::SendMessage(hEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)aURL);
	}

    return rv;
}


NS_IMPL_ISUPPORTS1( nsUrlWidget, nsIUrlWidget )

nsUrlWidget::nsUrlWidget() {
#ifdef DEBUG_URLWIDGET
printf( "nsUrlWidget ctor called\n" );
#endif
}


nsUrlWidget::~nsUrlWidget() {
#ifdef DEBUG_URLWIDGET
printf( "nsUrlWidget dtor called\n" );
#endif
}
