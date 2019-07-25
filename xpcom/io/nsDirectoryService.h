





































#ifndef nsDirectoryService_h___
#define nsDirectoryService_h___

#include "nsIDirectoryService.h"
#include "nsHashtable.h"
#include "nsILocalFile.h"
#include "nsISupportsArray.h"
#include "nsIAtom.h"

#define NS_XPCOM_INIT_CURRENT_PROCESS_DIR       "MozBinD"   // Can be used to set NS_XPCOM_CURRENT_PROCESS_DIR
                                                            
#define NS_DIRECTORY_SERVICE_CID  {0xf00152d0,0xb40b,0x11d3,{0x8c, 0x9c, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

class nsDirectoryService : public nsIDirectoryService,
                           public nsIProperties,
                           public nsIDirectoryServiceProvider2
{
  public:

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIPROPERTIES  

  NS_DECL_NSIDIRECTORYSERVICE

  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

  nsDirectoryService();
   ~nsDirectoryService();

  static nsresult RealInit();
  void RegisterCategoryProviders();

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  static nsDirectoryService* gService;

private:
    nsresult GetCurrentProcessDirectory(nsILocalFile** aFile);
    
    static bool ReleaseValues(nsHashKey* key, void* data, void* closure);
    nsSupportsHashtable mHashtable;
    nsCOMPtr<nsISupportsArray> mProviders;

public:

#define DIR_ATOM(name_, value_) static nsIAtom* name_;
#include "nsDirectoryServiceAtomList.h"
#undef DIR_ATOM

};


#endif

