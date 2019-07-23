





































#ifndef nsJVMAuthTools_h___
#define nsJVMAuthTools_h___

#include "nsAgg.h"
#include "nsIJVMAuthTools.h"

class nsAuthenticationInfoImp : public nsIAuthenticationInfo
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHENTICATIONINFO

    nsAuthenticationInfoImp(char* aUsername, char* aPassword);
    virtual ~nsAuthenticationInfoImp();
    
protected:
    char* mUserName;
    char* mPassWord;
  
};

class nsJVMAuthTools : public nsIJVMAuthTools 
{
public:

    NS_DECL_AGGREGATED
    NS_DECL_NSIJVMAUTHTOOLS
    
    nsJVMAuthTools(nsISupports* outer);
    virtual ~nsJVMAuthTools(void);
};

#endif 
