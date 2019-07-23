





































#ifndef nsPrintSessionX_h__
#define nsPrintSessionX_h__

#include "nsPrintSession.h"
#include "nsIPrintSessionX.h"





class nsPrintSessionX : public nsPrintSession,
                        public nsIPrintSessionX
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINTSESSIONX

  nsPrintSessionX();
  virtual ~nsPrintSessionX();
  
  nsresult Init();
  
protected:
  PMPrintSession    mSession;
};

#endif 
