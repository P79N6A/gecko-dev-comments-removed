




































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsIAppShell.h"
#include "nsIEventQueue.h"

#include <Pt.h>




class nsAppShell : public nsIAppShell
{
public:
  nsAppShell(); 
  virtual ~nsAppShell();

  NS_DECL_ISUPPORTS

	NS_IMETHOD Create(int *argc, char **argv);
	NS_IMETHOD Run(void);
	NS_IMETHOD Spinup(void);
	NS_IMETHOD Spindown(void);
	NS_IMETHOD ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen);
	inline NS_IMETHOD GetNativeEvent(PRBool & aRealEvent, void * & aEvent)
		{
		aRealEvent = PR_FALSE;
		aEvent = 0;
		return NS_OK;
		}

	inline NS_IMETHOD DispatchNativeEvent(PRBool aRealEvent, void * aEvent)
		{
		PtProcessEvent();

		return NS_OK;
		}

	NS_IMETHOD Exit(void);

public:
  static PRBool  gExitMainLoop;

private:
	nsCOMPtr<nsIEventQueue> mEventQueue;
  int	mFD;
	static PRBool mPtInited;

};

#endif 
