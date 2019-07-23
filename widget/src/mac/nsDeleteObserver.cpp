




































#include "nsDeleteObserver.h"
#include "nsVoidArray.h"


nsDeleteObserved::nsDeleteObserved(void* aObject)
{
	mDeleteObserverArray = nsnull;
	mObject = (aObject ? aObject : this);
}


nsDeleteObserved::~nsDeleteObserved()
{
	if (mDeleteObserverArray) {
		
		for (PRInt32 i = mDeleteObserverArray->Count() - 1; i >= 0; --i) {
			nsDeleteObserver* deleteObserver = static_cast<nsDeleteObserver*>(mDeleteObserverArray->ElementAt(i));
			if (deleteObserver)
				deleteObserver->NotifyDelete(mObject);
		}
		delete mDeleteObserverArray;
		mDeleteObserverArray = nsnull;
	}
}


PRBool nsDeleteObserved::AddDeleteObserver(nsDeleteObserver* aDeleteObserver)
{
	if (!mDeleteObserverArray)
		mDeleteObserverArray = new nsVoidArray();

	if (mDeleteObserverArray)
		return mDeleteObserverArray->AppendElement(aDeleteObserver);

	return PR_FALSE;
}


PRBool nsDeleteObserved::RemoveDeleteObserver(nsDeleteObserver* aDeleteObserver)
{
	if (mDeleteObserverArray)
		return mDeleteObserverArray->RemoveElement(aDeleteObserver);

	return PR_FALSE;
}
