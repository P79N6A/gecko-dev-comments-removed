
#ifde



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
