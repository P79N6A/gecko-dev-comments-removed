




































#ifndef nsWinRegItem_h__
#define nsWinRegItem_h__

#include "prtypes.h"

#include "nsSoftwareUpdate.h"
#include "nsInstallObject.h"
#include "nsWinReg.h"

PR_BEGIN_EXTERN_C

class nsWinRegItem : public nsInstallObject {

public:

  

  
  nsWinRegItem(nsWinReg*        regObj,
               PRInt32          root,
               PRInt32          action,
               const nsAString&  sub,
               const nsAString&  valname,
               const nsAString&  val,
               PRInt32*         aReturn);
  
  nsWinRegItem(nsWinReg*        regObj,
               PRInt32          root,
               PRInt32          action,
               const nsAString&  sub,
               const nsAString&  valname,
               PRInt32          val,
               PRInt32*         aReturn);
  
  virtual ~nsWinRegItem();

  PRInt32 Prepare(void);

  PRInt32 Complete();
  
  char* toString();
  
  void Abort();
  

  PRBool CanUninstall();
  PRBool RegisterPackageNode();
	  
private:
  
  
  
  nsWinReg* mReg;        
  PRInt32   mRootkey;
  PRInt32   mCommand;
  nsString* mSubkey;     
  nsString* mName;       
  void*     mValue;      
  
  

  nsString* keystr(PRInt32 root, nsString* subkey, nsString* name);

  char* itoa(PRInt32 n);
  void reverseString(char* s);
};

PR_END_EXTERN_C

#endif 
