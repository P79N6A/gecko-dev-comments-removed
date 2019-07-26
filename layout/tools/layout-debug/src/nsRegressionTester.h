





#ifndef nsRegressionTester_h__
#define nsRegressionTester_h__

#include "nsCOMPtr.h"

#include "nsILayoutRegressionTester.h"  
#include "nsILayoutDebugger.h"

class nsIDOMWindow;
class nsIDocShell;




class  nsRegressionTester : public nsILayoutRegressionTester
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILAYOUTREGRESSIONTESTER

  nsRegressionTester();

protected:
  virtual ~nsRegressionTester();
  nsresult    GetDocShellFromWindow(nsIDOMWindow* inWindow, nsIDocShell** outShell);
};



#endif 
