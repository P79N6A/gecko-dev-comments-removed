





#ifndef nsCollationWin_h__
#define nsCollationWin_h__


#include "nsICollation.h"
#include "nsCollation.h"  
#include "plstr.h"



class nsCollationWin MOZ_FINAL : public nsICollation {

protected:
  nsCollation   *mCollation;  
  uint32_t      mLCID;        

public: 
  nsCollationWin();
  ~nsCollationWin(); 

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif
