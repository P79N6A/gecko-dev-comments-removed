









































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: crashinject <PID>\n");
    return 1;
  }

  int pid = atoi(argv[1]);
  if (pid <= 0) {
    fprintf(stderr, "Usage: crashinject <PID>\n");
    return 1;
  }

  
  wchar_t filename[_MAX_PATH];
  if (GetModuleFileNameW(NULL, filename, sizeof(filename) / sizeof(wchar_t)) == 0)
    return 1;

  wchar_t* slash = wcsrchr(filename, L'\\');
  if (slash == NULL)
    return 1;

  slash++;
  wcscpy(slash, L"crashinjectdll.dll");

  
  HANDLE targetProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD,
                                  FALSE,
                                  pid);
  if (targetProc == NULL) {
    fprintf(stderr, "Error %d opening target process\n", GetLastError());
    return 1;
  }

  









  HMODULE hKernel32 = GetModuleHandleW(L"Kernel32");
  
  void*   pLibRemote = VirtualAllocEx(targetProc, NULL, sizeof(filename),
                                      MEM_COMMIT, PAGE_READWRITE);
  if (pLibRemote == NULL) {
    fprintf(stderr, "Error %d in VirtualAllocEx\n", GetLastError());
    CloseHandle(targetProc);
    return 1;
  }

  if (!WriteProcessMemory(targetProc, pLibRemote, (void*)filename,
                          sizeof(filename), NULL)) {
    fprintf(stderr, "Error %d in WriteProcessMemory\n", GetLastError());
    VirtualFreeEx(targetProc, pLibRemote, sizeof(filename), MEM_RELEASE);
    CloseHandle(targetProc);
    return 1;
  }
  
  HANDLE hThread = CreateRemoteThread(
                     targetProc, NULL, 0,
                     (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32,
                                                            "LoadLibraryW"),
                     pLibRemote, 0, NULL);
  if (hThread == NULL) {
    fprintf(stderr, "Error %d in CreateRemoteThread\n", GetLastError());
    VirtualFreeEx(targetProc, pLibRemote, sizeof(filename), MEM_RELEASE);
    CloseHandle(targetProc);
    return 1;
  }
  WaitForSingleObject(hThread, INFINITE);
  
  CloseHandle(hThread);
  VirtualFreeEx(targetProc, pLibRemote, sizeof(filename), MEM_RELEASE);
  CloseHandle(targetProc);

  return 0;
}
