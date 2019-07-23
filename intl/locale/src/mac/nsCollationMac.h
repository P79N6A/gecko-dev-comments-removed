





































#ifndef nsCollationMac_h__
#define nsCollationMac_h__


#include "nsICollation.h"
#include "nsCollation.h"  
#include "plstr.h"



class nsCollationMac : public nsICollation {

protected:
  nsCollation   *mCollation;            
  
  short         m_scriptcode;           
  unsigned char m_mac_sort_tbl[256];    

public: 
  nsCollationMac();
  ~nsCollationMac();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif
