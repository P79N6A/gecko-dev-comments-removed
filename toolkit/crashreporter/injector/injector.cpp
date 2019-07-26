



#include <windows.h>

#include "client/windows/handler/exception_handler.h"

using google_breakpad::ExceptionHandler;
using std::wstring;

extern "C" BOOL WINAPI DummyEntryPoint(HINSTANCE instance,
                                       DWORD reason,
                                       void* reserved)
{
  __debugbreak();

  return FALSE; 
}


extern "C" BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved); 

extern "C"
__declspec(dllexport) DWORD Start(void* context)
{
  
  
  _CRT_INIT(NULL, DLL_PROCESS_ATTACH, NULL);

  HANDLE hCrashPipe = reinterpret_cast<HANDLE>(context);

  ExceptionHandler* e = new (std::nothrow)
    ExceptionHandler(wstring(), NULL, NULL, NULL,
                     ExceptionHandler::HANDLER_ALL,
                     MiniDumpNormal, hCrashPipe, NULL);
  if (e)
    e->set_handle_debug_exceptions(true);
  return 1;
}
