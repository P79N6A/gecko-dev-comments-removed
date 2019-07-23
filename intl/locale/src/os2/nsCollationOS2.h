


































#ifndef _nscollationos2_h_
#define _nscollationos2_h_


#include "nsICollation.h"
#include "nsCollation.h"  
#include "nsCRT.h"



class nsCollationOS2 : public nsICollation {

protected:
  nsCollation   *mCollation;
  nsString      mLocale;
  nsString      mSavedLocale;

public:
  nsCollationOS2();
  ~nsCollationOS2();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif 
