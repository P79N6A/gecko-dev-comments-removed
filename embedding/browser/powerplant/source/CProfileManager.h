






































#pragma once


#include "nsError.h"


#include <LAttachment.h>


class nsIRegistry;

class CProfileManager : public LAttachment
{
  public:
                        CProfileManager();
    virtual             ~CProfileManager();
    
    virtual void        StartUp();
    virtual void        DoManageProfilesDialog();
    virtual Boolean     DoNewProfileDialog(char *outName, UInt32 bufSize);
    
    virtual void        DoLogout();
    
  protected:
  
    
    void                ExecuteSelf(MessageT inMessage, void *ioParam);
    
    nsresult            GetShowDialogOnStart(PRBool* showIt);
    nsresult            SetShowDialogOnStart(PRBool showIt);
    nsresult            OpenAppRegistry(nsIRegistry **aRegistry);
};
