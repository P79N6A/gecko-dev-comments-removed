























#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>


#ifdef NSIS
#include "exdll.h"
#endif


#import "netfw.tlb"
#include <netfw.h>
using namespace NetFwTypeLib;


#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )


#ifdef NSIS
HINSTANCE g_hInstance;
#endif

HRESULT     WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2);

HRESULT AddRule(LPCTSTR ExceptionName, LPCTSTR ProcessPath)
{
	HRESULT result = CoInitialize(NULL);
	if (FAILED(result))
		return result;
	result = REGDB_E_CLASSNOTREG;

	HRESULT hrComInit = S_OK;
	HRESULT hr = S_OK;

	INetFwPolicy2 *pNetFwPolicy2 = NULL;
	INetFwRules *pFwRules = NULL;
	INetFwRule *pFwRule = NULL;

	INetFwRule *pFwRuleExisting = NULL;




	BSTR bstrRuleName = SysAllocString(ExceptionName);
	BSTR bstrApplicationName = SysAllocString(ProcessPath);
	BSTR bstrRuleInterfaceType = SysAllocString(L"All");

	
	hrComInit = CoInitializeEx(
		0,
		COINIT_APARTMENTTHREADED
		);

	
	
	
	if (hrComInit != RPC_E_CHANGED_MODE)
	{
		if (FAILED(hrComInit))
		{
			printf("CoInitializeEx failed: 0x%08lx\n", hrComInit);
			goto Cleanup;
		}
	}

	
	hr = WFCOMInitialize(&pNetFwPolicy2);
	if (FAILED(hr))
	{
		try
		{
			INetFwMgrPtr fwMgr(L"HNetCfg.FwMgr");
			if (fwMgr)
			{
				INetFwAuthorizedApplicationPtr app(L"HNetCfg.FwAuthorizedApplication");
				if (app)
				{
					app->ProcessImageFileName = ProcessPath;
					app->Name = ExceptionName;
					app->Scope = NetFwTypeLib::NET_FW_SCOPE_ALL;
					app->IpVersion = NetFwTypeLib::NET_FW_IP_VERSION_ANY;
					app->Enabled = VARIANT_TRUE;
					fwMgr->LocalPolicy->CurrentProfile->AuthorizedApplications->Add(app);
				}
			}
		}
		catch (_com_error& e)
		{
			printf("%s", e.Error());
		}
		goto Cleanup;
	}

	
	hr = pNetFwPolicy2->get_Rules(&pFwRules);
	if (FAILED(hr))
	{
		printf("get_Rules failed: 0x%08lx\n", hr);
		goto Cleanup;
	}


	
	hr = pFwRules->Item(bstrRuleName, &pFwRuleExisting);
	
	if (pFwRuleExisting != NULL)
	{
		pFwRuleExisting->Release();
	}
	if (SUCCEEDED(hr))
	{
		printf("Firewall profile already exists\n");
		goto Cleanup;
	}

	







	
	






	
	hr = CoCreateInstance(
		__uuidof(NetFwRule),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(INetFwRule),
		(void**)&pFwRule);
	if (FAILED(hr))
	{
		printf("CoCreateInstance for Firewall Rule failed: 0x%08lx\n", hr);
		goto Cleanup;
	}

    
	pFwRule->put_Name(bstrRuleName);
	pFwRule->put_Protocol(NetFwTypeLib::NET_FW_IP_PROTOCOL_TCP);
	pFwRule->put_InterfaceTypes(bstrRuleInterfaceType);
	pFwRule->put_Profiles(NET_FW_PROFILE2_PRIVATE);
	pFwRule->put_Action(NET_FW_ACTION_ALLOW);
	pFwRule->put_Enabled(VARIANT_TRUE);

	pFwRule->put_ApplicationName(bstrApplicationName);
	
	hr = pFwRules->Add(pFwRule);
	if (FAILED(hr))
	{
		printf("Firewall Rule Add failed: 0x%08lx\n", hr);
		goto Cleanup;
	}

	pFwRule->Release();


	
	hr = CoCreateInstance(
		__uuidof(NetFwRule),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(INetFwRule),
		(void**)&pFwRule);
	if (FAILED(hr))
	{
		printf("CoCreateInstance for Firewall Rule failed: 0x%08lx\n", hr);
		goto Cleanup;
	}

	
	pFwRule->put_Name(bstrRuleName);


	pFwRule->put_Protocol(NetFwTypeLib::NET_FW_IP_PROTOCOL_UDP);

	pFwRule->put_InterfaceTypes(bstrRuleInterfaceType);


	pFwRule->put_Profiles(NET_FW_PROFILE2_PRIVATE);

	pFwRule->put_Action(NET_FW_ACTION_ALLOW);
	pFwRule->put_Enabled(VARIANT_TRUE);

	pFwRule->put_ApplicationName(bstrApplicationName);
	
	hr = pFwRules->Add(pFwRule);
	if (FAILED(hr))
	{
		printf("Firewall Rule Add failed: 0x%08lx\n", hr);
		goto Cleanup;
	}

Cleanup:

	
	SysFreeString(bstrRuleName);
	SysFreeString(bstrApplicationName);
	SysFreeString(bstrRuleInterfaceType);

	
	if (pFwRule != NULL)
	{
		pFwRule->Release();
	}

	
	if (pFwRules != NULL)
	{
		pFwRules->Release();
	}

	
	if (pNetFwPolicy2 != NULL)
	{
		pNetFwPolicy2->Release();
	}

	CoUninitialize();
    return 0;
}

HRESULT RemoveRule(LPCTSTR ExceptionName, LPCTSTR ProcessPath)
{
    HRESULT result = CoInitialize(NULL);
	if (FAILED(result))
        return result;
	try
	{
		INetFwMgrPtr fwMgr(L"HNetCfg.FwMgr");
        if (fwMgr)
        {
		    fwMgr->LocalPolicy->CurrentProfile->AuthorizedApplications->Remove(ProcessPath);
            result = S_OK;
        }
	}
	catch (_com_error& e)
	{
		e;
	}
	HRESULT hrComInit = S_OK;
	HRESULT hr = S_OK;

	INetFwPolicy2 *pNetFwPolicy2 = NULL;
	INetFwRules *pFwRules = NULL;





	BSTR bstrRuleName = SysAllocString(ExceptionName);

	
	hr = WFCOMInitialize(&pNetFwPolicy2);
	if (FAILED(hr))
	{
		goto Cleanup;
	}

	
	hr = pNetFwPolicy2->get_Rules(&pFwRules);
	if (FAILED(hr))
	{
		printf("get_Rules failed: 0x%08lx\n", hr);
		goto Cleanup;
	}


	







	
	







	
	hr = pFwRules->Remove(bstrRuleName);
	if (FAILED(hr))
	{
		printf("Firewall Rule Remove failed: 0x%08lx\n", hr);
		goto Cleanup;
	}

Cleanup:

	
	SysFreeString(bstrRuleName);

	
	if (pFwRules != NULL)
	{
		pFwRules->Release();
	}

	
	if (pNetFwPolicy2 != NULL)
	{
		pNetFwPolicy2->Release();
	}

	CoUninitialize();
	return 0;
}


#ifdef NSIS
extern "C" void __declspec(dllexport) AddRule(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	
	TCHAR ExceptionName[256], ProcessPath[MAX_PATH];
    popstring(ProcessPath);
    popstring(ExceptionName);
    HRESULT result = AddRule(ExceptionName, ProcessPath);
	
    TCHAR intBuffer[16];
	wsprintf(intBuffer, _T("%d"), result);
	pushstring(intBuffer);
}

extern "C" void __declspec(dllexport) RemoveRule(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	
	TCHAR ExceptionName[256], ProcessPath[MAX_PATH];
    popstring(ProcessPath);
	popstring(ExceptionName);
    HRESULT result = RemoveRule(ExceptionName, ProcessPath);
	
    TCHAR intBuffer[16];
	wsprintf(intBuffer, _T("%d"), result);
	pushstring(intBuffer);
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD, LPVOID)
{
	g_hInstance = hInstance;
	return TRUE;
}
#endif




HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2)
{
	HRESULT hr = S_OK;

	hr = CoCreateInstance(
		__uuidof(NetFwPolicy2), 
		NULL, 
		CLSCTX_INPROC_SERVER, 
		__uuidof(INetFwPolicy2), 
		(void**)ppNetFwPolicy2);

	if (FAILED(hr))
	{
		printf("CoCreateInstance for INetFwPolicy2 failed: 0x%08lx\n", hr);
		goto Cleanup;        
	}

Cleanup:
	return hr;
}
