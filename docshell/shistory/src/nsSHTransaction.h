






































#ifndef nsSHTransaction_h
#define nsSHTransaction_h


#include "nsCOMPtr.h"


#include "nsISHTransaction.h"
#include "nsISHEntry.h"

class nsSHTransaction: public nsISHTransaction
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSISHTRANSACTION

	nsSHTransaction();

protected:
	virtual ~nsSHTransaction();


protected:
   PRBool         mPersist;

	nsISHTransaction * mPrev; 
	nsCOMPtr<nsISHTransaction> mNext;
	nsCOMPtr<nsISHEntry>  mSHEntry;
};


#endif   
