




































#ifndef _FILESPECIMPL_H_
#define _FILESPECIMPL_H_

#include "nscore.h"
#include "nsIFileSpec.h" 
#include "nsFileSpec.h"


class nsFileSpecImpl

	: public nsIFileSpec
{

 public: 

	NS_DECL_ISUPPORTS

  NS_DECL_NSIFILESPEC

	
	
	

	static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aIFileSpec);

	
	
	

	nsFileSpecImpl();
	nsFileSpecImpl(const nsFileSpec& inSpec);
	static nsresult MakeInterface(const nsFileSpec& inSpec, nsIFileSpec** outSpec);

	
	
	

	nsFileSpec							mFileSpec;
	nsIInputStream*					mInputStream;
	nsIOutputStream*				mOutputStream;

private:
	~nsFileSpecImpl();
}; 


class nsDirectoryIteratorImpl

	: public nsIDirectoryIterator
{

public:

	nsDirectoryIteratorImpl();

	NS_DECL_ISUPPORTS

	NS_IMETHOD Init(nsIFileSpec *parent, PRBool resolveSymlink);

	NS_IMETHOD Exists(PRBool *_retval);

	NS_IMETHOD Next();

	NS_IMETHOD GetCurrentSpec(nsIFileSpec * *aCurrentSpec);

	
	
	

	static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aIFileSpec);

private:
	~nsDirectoryIteratorImpl();

protected:
	nsDirectoryIterator*					mDirectoryIterator;
}; 

#endif 
