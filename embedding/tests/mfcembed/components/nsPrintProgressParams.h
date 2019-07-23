






























#ifndef __nsPrintProgressParams_h
#define __nsPrintProgressParams_h

#include "nsIPrintProgressParams.h"
#include "nsString.h"

class nsPrintProgressParams : public nsIPrintProgressParams
{
public: 
	NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTPROGRESSPARAMS

	nsPrintProgressParams();
	virtual ~nsPrintProgressParams();

private:
  nsString mDocTitle;
  nsString mDocURL;
};

#endif
