




































#ifndef nsGREDirServiceProvider_h_
#define nsGREDirServiceProvider_h_

#include "nsIDirectoryService.h"




class nsGREDirServiceProvider : public nsIDirectoryServiceProvider
{
public:
   nsGREDirServiceProvider() { }

   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER

private:
   ~nsGREDirServiceProvider() { }
};

#endif 
