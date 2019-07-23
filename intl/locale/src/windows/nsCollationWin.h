





































#ifndef nsCollationWin_h__
#define nsCollationWin_h__


#include "nsICollation.h"
#include "nsCollation.h"  
#include "plstr.h"



class nsCollationWin : public nsICollation {

protected:
  nsCollation   *mCollation;  
  PRUint32      mLCID;        

public: 
  nsCollationWin();
  ~nsCollationWin(); 

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif
