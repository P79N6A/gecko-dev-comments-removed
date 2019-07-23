












































#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "MozillaControl.h"

#include "MozillaControl_i.c"
#include "MozillaBrowser.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MozillaBrowser, CMozillaBrowser)
END_OBJECT_MAP()




extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    NG_TRACE_METHOD(DllMain);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        NG_TRACE(_T("Mozilla ActiveX - DLL_PROCESS_ATTACH\n"));
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        NG_TRACE(_T("Mozilla ActiveX - DLL_PROCESS_DETACH\n"));
        _Module.Term();
    }

    return TRUE;    
}




STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}




STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}




STDAPI DllRegisterServer(void)
{
    
    return _Module.RegisterServer(TRUE);
}




STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


