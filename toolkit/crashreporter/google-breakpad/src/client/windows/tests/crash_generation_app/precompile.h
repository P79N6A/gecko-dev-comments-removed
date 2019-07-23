

































#ifndef CLIENT_WINDOWS_TESTS_CRASH_GENERATION_APP_PRECOMPILE_H__
#define CLIENT_WINDOWS_TESTS_CRASH_GENERATION_APP_PRECOMPILE_H__






#ifndef WINVER

#define WINVER 0x0501
#endif


#ifndef _WIN32_WINNT

#define _WIN32_WINNT 0x0501
#endif


#ifndef _WIN32_WINDOWS

#define _WIN32_WINDOWS 0x0410
#endif


#ifndef _WIN32_IE

#define _WIN32_IE 0x0600
#endif


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <DbgHelp.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <cassert>
#include <list>

#include "client/windows/common/ipc_protocol.h"
#include "client/windows/crash_generation/client_info.h"
#include "client/windows/crash_generation/crash_generation_client.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/crash_generation/minidump_generator.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/tests/crash_generation_app/abstract_class.h"
#include "client/windows/tests/crash_generation_app/crash_generation_app.h"
#include "google_breakpad/common/minidump_format.h"

#endif  
