




































#ifndef nsWinProfileItem_h__
#define nsWinProfileItem_h__

#include "prtypes.h"
#include "nsSoftwareUpdate.h"
#include "nsInstallObject.h"
#include "nsWinProfile.h"


PR_BEGIN_EXTERN_C

class nsWinProfileItem : public nsInstallObject {

public:

  

  

  nsWinProfileItem(nsWinProfile* profileObj,
                   nsString sectionName,
                   nsString keyName,
                   nsString val,
                   PRInt32 *aReturn);

  virtual ~nsWinProfileItem();

  



  PRInt32 Complete();
  char* toString();
  
  
  void Abort();
  
  
  PRInt32 Prepare();
  
  
  PRBool CanUninstall();
  PRBool RegisterPackageNode();
  
private:
  
  
  nsWinProfile* mProfile;     
  nsString*     mSection;     
  nsString*     mKey;         
  nsString*     mValue;       
  
  
 
};

PR_END_EXTERN_C

#endif 
