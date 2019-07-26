





#ifndef nsSHTransaction_h
#define nsSHTransaction_h


#include "nsCOMPtr.h"


#include "nsISHTransaction.h"

class nsISHEntry;

class nsSHTransaction: public nsISHTransaction
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSISHTRANSACTION

	nsSHTransaction();

protected:
	virtual ~nsSHTransaction();


protected:
   bool           mPersist;

	nsISHTransaction * mPrev; 
	nsCOMPtr<nsISHTransaction> mNext;
	nsCOMPtr<nsISHEntry>  mSHEntry;
};


#endif   
