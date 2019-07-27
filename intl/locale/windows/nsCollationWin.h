





#ifndef nsCollationWin_h__
#define nsCollationWin_h__


#include "nsICollation.h"
#include "nsCollation.h"  
#include "plstr.h"



class nsCollationWin final : public nsICollation {
  ~nsCollationWin();

protected:
  nsCollation   *mCollation;  
  uint32_t      mLCID;        

public: 
  nsCollationWin();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif
