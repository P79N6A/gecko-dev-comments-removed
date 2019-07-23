









































#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "UrlDialog.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"
#include "QaUtils.h"
#include "nsiDirServ.h"
#include "nsIDirectoryService.h"
#include "winEmbedFileLocProvider.h"
#include <stdio.h>





CNsIDirectoryService::CNsIDirectoryService()
{

}










CNsIDirectoryService::~CNsIDirectoryService()
{
}















void CNsIDirectoryService::StartTests(UINT nMenuID)
{

    QAOutput("------------------------------------------------",1);
    QAOutput("Start DirectoryService Test",1);
   	
	
	

	switch(nMenuID)
	{
	case ID_INTERFACES_NSIDIRECTORYSERVICE_RUNALLTESTS :
			  Init(); 
			  RegisterProvider(); 	
			  UnRegisterProvider(); 	
			  break ;
	case ID_INTERFACES_NSIDIRECTORYSERVICE_INIT :
			  Init(); 
			  break ;
	case ID_INTERFACES_NSIDIRECTORYSERVICE_REGISTERPROVIDER :
			  RegisterProvider(); 	
			  break ;
	case ID_INTERFACES_NSIDIRECTORYSERVICE_UNREGISTERPROVIDER :
			  UnRegisterProvider(); 	
			  break ;
	default :  
			AfxMessageBox("Menu handler not added for this menu item");
			break ;
	}	
}

void CNsIDirectoryService::Init()
{
   nsCOMPtr<nsIDirectoryService> theDirService(do_CreateInstance(NS_DIRECTORY_SERVICE_CONTRACTID));

    if (!theDirService)
 	{
		QAOutput("Directory Service object doesn't exist.", 0);
		return;
	}

	rv = theDirService->Init();
	RvTestResult(rv, "rv Init() test", 1);
	RvTestResultDlg(rv, "rv Init() test", true);
}
void CNsIDirectoryService::RegisterProvider()
{
   nsCOMPtr<nsIDirectoryService> theDirService(do_CreateInstance(NS_DIRECTORY_SERVICE_CONTRACTID));
   winEmbedFileLocProvider *provider = new winEmbedFileLocProvider("TestEmbed");
    if (!theDirService)
 	{
		QAOutput("Directory Service object doesn't exist.", 0);
		return;
	}
    if (!provider)
 	{
		QAOutput("Directory Service Provider object doesn't exist.", 0);
		return;
	}
	rv= theDirService->RegisterProvider(provider);
	RvTestResult(rv, "rv RegisterProvider() test", 1);
	RvTestResultDlg(rv, "rv RegisterProvider() test");
}
void CNsIDirectoryService::UnRegisterProvider()
{
   nsCOMPtr<nsIDirectoryService> theDirService(do_CreateInstance(NS_DIRECTORY_SERVICE_CONTRACTID));
   winEmbedFileLocProvider *provider = new winEmbedFileLocProvider("TestEmbed");
    if (!theDirService)
 	{
		QAOutput("Directory Service object doesn't exist.", 0);
		return;
	}
    if (!provider)
 	{
		QAOutput("Directory Service Provider object doesn't exist.", 0);
		return;
	}
	rv= theDirService->RegisterProvider(provider);
	rv= theDirService->UnregisterProvider(provider);
	RvTestResult(rv, "rv UnRegisterProvider() test", 1);
	RvTestResultDlg(rv, "rv UnRegisterProvider() test");
}


