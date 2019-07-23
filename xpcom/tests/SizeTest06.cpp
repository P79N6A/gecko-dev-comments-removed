

#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsCOMPtr.h"

NS_DEF_PTR(nsPIDOMWindow);
NS_DEF_PTR(nsIBaseWindow);

  
























void 
Test06_raw(nsIDOMWindowInternal* aDOMWindow, nsIBaseWindow** aBaseWindow)
    
{


  nsPIDOMWindow* window = 0;
  nsresult status = aDOMWindow->QueryInterface(NS_GET_IID(nsPIDOMWindow), (void**)&window);
  nsIDocShell* docShell = 0;
  if (window)
    window->GetDocShell(&docShell);
  nsIWebShell* rootWebShell = 0;
  NS_IF_RELEASE(rootWebShell);
  NS_IF_RELEASE(docShell);
  NS_IF_RELEASE(window);

}

void 
Test06_raw_optimized(nsIDOMWindowInternal* aDOMWindow, nsIBaseWindow** aBaseWindow)
    
{


  (*aBaseWindow) = 0;
  nsPIDOMWindow* window;
  nsresult status = aDOMWindow->QueryInterface(NS_GET_IID(nsPIDOMWindow), (void**)&window);
  if (NS_SUCCEEDED(status)) {
    nsIDocShell* docShell = 0;
    window->GetDocShell(&docShell);
    if (docShell) {
      NS_RELEASE(docShell);
    }
    NS_RELEASE(window);
  }

}

void
Test06_nsCOMPtr_as_found(nsIDOMWindowInternal* aDOMWindow, nsCOMPtr<nsIBaseWindow>* aBaseWindow)
    
{


  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow);
  nsCOMPtr<nsIDocShell> docShell;
  if (window)
    window->GetDocShell(getter_AddRefs(docShell));  
}

void 
Test06_nsCOMPtr00(nsIDOMWindowInternal* aDOMWindow, nsIBaseWindow** aBaseWindow)
    
{


  nsresult status;
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow, &status);
  nsIDocShell* temp0 = 0;
  if (window)
    window->GetDocShell(&temp0);
  nsCOMPtr<nsIDocShell> docShell = dont_AddRef(temp0);
  (*aBaseWindow) = 0;

}

void 
Test06_nsCOMPtr_optimized(nsIDOMWindowInternal* aDOMWindow, nsCOMPtr<nsIBaseWindow>* aBaseWindow)
    
{


  nsresult status;
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow, &status);
  nsIDocShell* temp0 = 0;
  if (window)
    window->GetDocShell(&temp0);
  (*aBaseWindow) = do_QueryInterface(nsnull, &status);

}

void 
Test06_nsCOMPtr02(nsIDOMWindowInternal* aDOMWindow, nsIBaseWindow** aBaseWindow)
    
{


  (*aBaseWindow) = 0;
  nsresult status;
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow, &status);
  if (window) {
    nsIDocShell* temp0;
    window->GetDocShell(&temp0);
  }

}

void 
Test06_nsCOMPtr03(nsIDOMWindowInternal* aDOMWindow, nsCOMPtr<nsIBaseWindow>* aBaseWindow)
    
{


  (*aBaseWindow) = 0;
  nsresult status;
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow, &status);
  if (window) {
    nsIDocShell* temp0;
    window->GetDocShell(&temp0);
    nsCOMPtr<nsIDocShell> docShell = dont_AddRef(temp0);
    if (docShell) {
    }
  }

}
