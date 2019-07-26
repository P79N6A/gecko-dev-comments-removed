































#include "client/windows/tests/crash_generation_app/crash_generation_app.h"

#include <windows.h>
#include <tchar.h>

#include "client/windows/crash_generation/client_info.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/common/ipc_protocol.h"

#include "client/windows/tests/crash_generation_app/abstract_class.h"

namespace google_breakpad {

const int kMaxLoadString = 100;
const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashServices\\TestServer";

const DWORD kEditBoxStyles = WS_CHILD |
                             WS_VISIBLE |
                             WS_VSCROLL |
                             ES_LEFT |
                             ES_MULTILINE |
                             ES_AUTOVSCROLL |
                             ES_READONLY;


const size_t kMaximumLineLength = 256;


static CRITICAL_SECTION* cs_edit = NULL;


static HWND client_status_edit_box;

HINSTANCE current_instance;             
TCHAR title[kMaxLoadString];            
TCHAR window_class[kMaxLoadString];     

ATOM MyRegisterClass(HINSTANCE instance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static int kCustomInfoCount = 2;
static CustomInfoEntry kCustomInfoEntries[] = {
    CustomInfoEntry(L"prod", L"CrashTestApp"),
    CustomInfoEntry(L"ver", L"1.0"),
};

static ExceptionHandler* handler = NULL;
static CrashGenerationServer* crash_server = NULL;








ATOM MyRegisterClass(HINSTANCE instance) {
  WNDCLASSEX wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = instance;
  wcex.hIcon = LoadIcon(instance,
                        MAKEINTRESOURCE(IDI_CRASHGENERATIONAPP));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CRASHGENERATIONAPP);
  wcex.lpszClassName = window_class;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassEx(&wcex);
}





BOOL InitInstance(HINSTANCE instance, int command_show) {
  current_instance = instance;
  HWND wnd = CreateWindow(window_class,
                          title,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          0,
                          CW_USEDEFAULT,
                          0,
                          NULL,
                          NULL,
                          instance,
                          NULL);

  if (!wnd) {
    return FALSE;
  }

  ShowWindow(wnd, command_show);
  UpdateWindow(wnd);

  return TRUE;
}

static void AppendTextToEditBox(TCHAR* text) {
  EnterCriticalSection(cs_edit);
  SYSTEMTIME current_time;
  GetLocalTime(&current_time);
  TCHAR line[kMaximumLineLength];
  int result = swprintf_s(line,
                          kMaximumLineLength,
                          L"[%.2d-%.2d-%.4d %.2d:%.2d:%.2d] %s",
                          current_time.wMonth,
                          current_time.wDay,
                          current_time.wYear,
                          current_time.wHour,
                          current_time.wMinute,
                          current_time.wSecond,
                          text);

  if (result == -1) {
    return;
  }

  int length = GetWindowTextLength(client_status_edit_box);
  SendMessage(client_status_edit_box,
              EM_SETSEL,
              (WPARAM)length,
              (LPARAM)length);
  SendMessage(client_status_edit_box,
              EM_REPLACESEL,
              (WPARAM)FALSE,
              (LPARAM)line);
  LeaveCriticalSection(cs_edit);
}

static DWORD WINAPI AppendTextWorker(void* context) {
  TCHAR* text = reinterpret_cast<TCHAR*>(context);

  AppendTextToEditBox(text);
  delete[] text;

  return 0;
}

bool ShowDumpResults(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded) {
  TCHAR* text = new TCHAR[kMaximumLineLength];
  text[0] = _T('\0');
  int result = swprintf_s(text,
                          kMaximumLineLength,
                          TEXT("Dump generation request %s\r\n"),
                          succeeded ? TEXT("succeeded") : TEXT("failed"));
  if (result == -1) {
    delete [] text;
  }

  QueueUserWorkItem(AppendTextWorker, text, WT_EXECUTEDEFAULT);
  return succeeded;
}

static void _cdecl ShowClientConnected(void* context,
                                       const ClientInfo* client_info) {
  TCHAR* line = new TCHAR[kMaximumLineLength];
  line[0] = _T('\0');
  int result = swprintf_s(line,
                          kMaximumLineLength,
                          L"Client connected:\t\t%d\r\n",
                          client_info->pid());

  if (result == -1) {
    delete[] line;
    return;
  }

  QueueUserWorkItem(AppendTextWorker, line, WT_EXECUTEDEFAULT);
}

static void _cdecl ShowClientCrashed(void* context,
                                     const ClientInfo* client_info,
                                     const wstring* dump_path) {
  TCHAR* line = new TCHAR[kMaximumLineLength];
  line[0] = _T('\0');
  int result = swprintf_s(line,
                          kMaximumLineLength,
                          TEXT("Client requested dump:\t%d\r\n"),
                          client_info->pid());

  if (result == -1) {
    delete[] line;
    return;
  }

  QueueUserWorkItem(AppendTextWorker, line, WT_EXECUTEDEFAULT);

  CustomClientInfo custom_info = client_info->GetCustomInfo();
  if (custom_info.count <= 0) {
    return;
  }

  wstring str_line;
  for (size_t i = 0; i < custom_info.count; ++i) {
    if (i > 0) {
      str_line += L", ";
    }
    str_line += custom_info.entries[i].name;
    str_line += L": ";
    str_line += custom_info.entries[i].value;
  }

  line = new TCHAR[kMaximumLineLength];
  line[0] = _T('\0');
  result = swprintf_s(line,
                      kMaximumLineLength,
                      L"%s\n",
                      str_line.c_str());
  if (result == -1) {
    delete[] line;
    return;
  }
  QueueUserWorkItem(AppendTextWorker, line, WT_EXECUTEDEFAULT);
}

static void _cdecl ShowClientExited(void* context,
                                    const ClientInfo* client_info) {
  TCHAR* line = new TCHAR[kMaximumLineLength];
  line[0] = _T('\0');
  int result = swprintf_s(line,
                          kMaximumLineLength,
                          TEXT("Client exited:\t\t%d\r\n"),
                          client_info->pid());

  if (result == -1) {
    delete[] line;
    return;
  }

  QueueUserWorkItem(AppendTextWorker, line, WT_EXECUTEDEFAULT);
}

void CrashServerStart() {
  
  if (crash_server) {
    return;
  }

  std::wstring dump_path = L"C:\\Dumps\\";
  crash_server = new CrashGenerationServer(kPipeName,
                                           NULL,
                                           ShowClientConnected,
                                           NULL,
                                           ShowClientCrashed,
                                           NULL,
                                           ShowClientExited,
                                           NULL,
                                           NULL,
                                           NULL,
                                           true,
                                           &dump_path);

  if (!crash_server->Start()) {
    MessageBoxW(NULL, L"Unable to start server", L"Dumper", MB_OK);
    delete crash_server;
    crash_server = NULL;
  }
}

void CrashServerStop() {
  delete crash_server;
  crash_server = NULL;
}

void DerefZeroCrash() {
  int* x = 0;
  *x = 1;
}

void InvalidParamCrash() {
  printf(NULL);
}

void PureCallCrash() {
  Derived derived;
}

void RequestDump() {
  if (!handler->WriteMinidump()) {
    MessageBoxW(NULL, L"Dump request failed", L"Dumper", MB_OK);
  }
  kCustomInfoEntries[1].set_value(L"1.1");
}

void CleanUp() {
  if (cs_edit) {
    DeleteCriticalSection(cs_edit);
    delete cs_edit;
  }

  if (handler) {
    delete handler;
  }

  if (crash_server) {
    delete crash_server;
  }
}






LRESULT CALLBACK WndProc(HWND wnd,
                         UINT message,
                         WPARAM w_param,
                         LPARAM l_param) {
  int message_id;
  int message_event;
  PAINTSTRUCT ps;
  HDC hdc;

#pragma warning(push)
#pragma warning(disable:4312)
  
  
  
  HINSTANCE instance = (HINSTANCE)GetWindowLong(wnd, GWL_HINSTANCE);
#pragma warning(pop)

  switch (message) {
    case WM_COMMAND:
      
      message_id = LOWORD(w_param);
      message_event = HIWORD(w_param);
      switch (message_id) {
        case IDM_ABOUT:
          DialogBox(current_instance,
                    MAKEINTRESOURCE(IDD_ABOUTBOX),
                    wnd,
                    About);
          break;
        case IDM_EXIT:
          DestroyWindow(wnd);
          break;
        case ID_SERVER_START:
          CrashServerStart();
          break;
        case ID_SERVER_STOP:
          CrashServerStop();
          break;
        case ID_CLIENT_DEREFZERO:
          DerefZeroCrash();
          break;
        case ID_CLIENT_INVALIDPARAM:
          InvalidParamCrash();
          break;
        case ID_CLIENT_PURECALL:
          PureCallCrash();
          break;
        case ID_CLIENT_REQUESTEXPLICITDUMP:
          RequestDump();
          break;
        default:
          return DefWindowProc(wnd, message, w_param, l_param);
      }
      break;
    case WM_CREATE:
      client_status_edit_box = CreateWindow(TEXT("EDIT"),
                                            NULL,
                                            kEditBoxStyles,
                                            0,
                                            0,
                                            0,
                                            0,
                                            wnd,
                                            NULL,
                                            instance,
                                            NULL);
      break;
    case WM_SIZE: 
      
      MoveWindow(client_status_edit_box, 
                 0,
                 0,
                 LOWORD(l_param),        
                 HIWORD(l_param),        
                 TRUE);                  
      break;
    case WM_SETFOCUS: 
      SetFocus(client_status_edit_box);
      break;
    case WM_PAINT:
      hdc = BeginPaint(wnd, &ps);
      EndPaint(wnd, &ps);
      break;
    case WM_DESTROY:
      CleanUp();
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(wnd, message, w_param, l_param);
  }

  return 0;
}


INT_PTR CALLBACK About(HWND dlg,
                       UINT message,
                       WPARAM w_param,
                       LPARAM l_param) {
  UNREFERENCED_PARAMETER(l_param);
  switch (message) {
    case WM_INITDIALOG:
      return (INT_PTR)TRUE;

    case WM_COMMAND:
      if (LOWORD(w_param) == IDOK || LOWORD(w_param) == IDCANCEL) {
        EndDialog(dlg, LOWORD(w_param));
        return (INT_PTR)TRUE;
      }
      break;
  }

  return (INT_PTR)FALSE;
}

}  

int APIENTRY _tWinMain(HINSTANCE instance,
                       HINSTANCE previous_instance,
                       LPTSTR command_line,
                       int command_show) {
  using namespace google_breakpad;

  UNREFERENCED_PARAMETER(previous_instance);
  UNREFERENCED_PARAMETER(command_line);

  cs_edit = new CRITICAL_SECTION();
  InitializeCriticalSection(cs_edit);

  CustomClientInfo custom_info = {kCustomInfoEntries, kCustomInfoCount};

  CrashServerStart();
  
  
  _CrtSetReportMode(_CRT_ASSERT, 0);
  handler = new ExceptionHandler(L"C:\\dumps\\",
                                 NULL,
                                 google_breakpad::ShowDumpResults,
                                 NULL,
                                 ExceptionHandler::HANDLER_ALL,
                                 MiniDumpNormal,
                                 kPipeName,
                                 &custom_info);

  
  LoadString(instance, IDS_APP_TITLE, title, kMaxLoadString);
  LoadString(instance,
             IDC_CRASHGENERATIONAPP,
             window_class,
             kMaxLoadString);
  MyRegisterClass(instance);

  
  if (!InitInstance (instance, command_show)) {
    return FALSE;
  }

  HACCEL accel_table = LoadAccelerators(
      instance,
      MAKEINTRESOURCE(IDC_CRASHGENERATIONAPP));

  
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, accel_table, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}
