





























#ifndef __ProfileMgr__
#define __ProfileMgr__


#include "nsError.h"
 

class nsIRegistry;





class CProfileMgr
{
  public:
                        CProfileMgr();
    virtual             ~CProfileMgr();
    
    virtual nsresult    StartUp();
    virtual nsresult    DoManageProfilesDialog(PRBool bAtStartUp);
        
    
  protected:
      
    nsresult            GetShowDialogOnStart(PRBool* showIt);
    nsresult            SetShowDialogOnStart(PRBool showIt);
};




#endif
