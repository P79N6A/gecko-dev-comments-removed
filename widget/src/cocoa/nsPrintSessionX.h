





































#ifndef nsPrintSessionX_h_
#define nsPrintSessionX_h_

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
