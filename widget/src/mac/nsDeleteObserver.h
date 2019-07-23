




































#ifndef DeleteObserver_h__
#define DeleteObserver_h__

#include "prtypes.h"
class nsVoidArray;










class nsDeleteObserver
{
public:
		virtual void	NotifyDelete(void* aDeletedObject) = 0;
};


class nsDeleteObserved
{
public:
							nsDeleteObserved(void* aObject);
	virtual			~nsDeleteObserved();

	PRBool			AddDeleteObserver(nsDeleteObserver* aDeleteObserver);
	PRBool			RemoveDeleteObserver(nsDeleteObserver* aDeleteObserver);

private:
	nsVoidArray*			mDeleteObserverArray;
	void*							mObject;
};

#endif 
