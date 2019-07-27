











































#include <windows.h>
#include <dbghelp.h>
#include <wininet.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "common/windows/string_utils-inl.h"

#include "common/windows/http_upload.h"
#include "common/windows/pdb_source_line_writer.h"

using std::string;
using std::wstring;
using std::vector;
using std::map;
using google_breakpad::HTTPUpload;
using google_breakpad::PDBModuleInfo;
using google_breakpad::PDBSourceLineWriter;
using google_breakpad::WindowsStringUtils;



static bool GetFileVersionString(const wchar_t *filename, wstring *version) {
  DWORD handle;
  DWORD version_size = GetFileVersionInfoSize(filename, &handle);
  if (version_size < sizeof(VS_FIXEDFILEINFO)) {
    return false;
  }

  vector<char> version_info(version_size);
  if (!GetFileVersionInfo(filename, handle, version_size, &version_info[0])) {
    return false;
  }

  void *file_info_buffer = NULL;
  unsigned int file_info_length;
  if (!VerQueryValue(&version_info[0], L"\\",
                     &file_info_buffer, &file_info_length)) {
    return false;
  }

  
  
  wchar_t ver_string[24];
  VS_FIXEDFILEINFO *file_info =
    reinterpret_cast<VS_FIXEDFILEINFO*>(file_info_buffer);
  swprintf(ver_string, sizeof(ver_string) / sizeof(ver_string[0]),
           L"%d.%d.%d.%d",
           file_info->dwFileVersionMS >> 16,
           file_info->dwFileVersionMS & 0xffff,
           file_info->dwFileVersionLS >> 16,
           file_info->dwFileVersionLS & 0xffff);

  
  ver_string[sizeof(ver_string) / sizeof(ver_string[0]) - 1] = L'\0';

  *version = ver_string;
  return true;
}




static bool DumpSymbolsToTempFile(const wchar_t *file,
                                  wstring *temp_file_path,
                                  PDBModuleInfo *pdb_info) {
  google_breakpad::PDBSourceLineWriter writer;
  
  
  
  if (!writer.Open(file, PDBSourceLineWriter::EXE_FILE)) {
    return false;
  }

  wchar_t temp_path[_MAX_PATH];
  if (GetTempPath(_MAX_PATH, temp_path) == 0) {
    return false;
  }

  wchar_t temp_filename[_MAX_PATH];
  if (GetTempFileName(temp_path, L"sym", 0, temp_filename) == 0) {
    return false;
  }

  FILE *temp_file = NULL;
#if _MSC_VER >= 1400  
  if (_wfopen_s(&temp_file, temp_filename, L"w") != 0)
#else  
  
  
  if (!(temp_file = _wfopen(temp_filename, L"w")))
#endif  
  {
    return false;
  }

  bool success = writer.WriteMap(temp_file);
  fclose(temp_file);
  if (!success) {
    _wunlink(temp_filename);
    return false;
  }

  *temp_file_path = temp_filename;

  return writer.GetModuleInfo(pdb_info);
}

void printUsageAndExit() {
  wprintf(L"Usage: symupload [--timeout NN] <file.exe|file.dll> <symbol upload URL>\n\n");
  wprintf(L"Timeout is in milliseconds, or can be 0 to be unlimited\n\n");
  wprintf(L"Example:\n\n\tsymupload.exe --timeout 0 chrome.dll http://no.free.symbol.server.for.you\n");
  exit(0);
}
int wmain(int argc, wchar_t *argv[]) {
  if ((argc != 3) &&
      (argc != 5)) {
    printUsageAndExit();
  }

  const wchar_t *module, *url;
  int timeout = -1;
  if (argc == 3) {
    module = argv[1];
    url = argv[2];
  } else {
    
    if (!wcscmp(L"--timeout", argv[1])) {
      timeout  = _wtoi(argv[2]);
      module = argv[3];
      url = argv[4];
    } else {
      printUsageAndExit();
    }
  }

  wstring symbol_file;
  PDBModuleInfo pdb_info;
  if (!DumpSymbolsToTempFile(module, &symbol_file, &pdb_info)) {
    fwprintf(stderr, L"Could not get symbol data from %s\n", module);
    return 1;
  }

  wstring code_file = WindowsStringUtils::GetBaseName(wstring(module));

  map<wstring, wstring> parameters;
  parameters[L"code_file"] = code_file;
  parameters[L"debug_file"] = pdb_info.debug_file;
  parameters[L"debug_identifier"] = pdb_info.debug_identifier;
  parameters[L"os"] = L"windows";  
  parameters[L"cpu"] = pdb_info.cpu;

  
  
  wstring file_version;
  if (GetFileVersionString(module, &file_version)) {
    parameters[L"version"] = file_version;
  } else {
    fwprintf(stderr, L"Warning: Could not get file version for %s\n", module);
  }

  bool success = HTTPUpload::SendRequest(url, parameters,
                                         symbol_file, L"symbol_file",
										 timeout == -1 ? NULL : &timeout,
                                         NULL, NULL);
  _wunlink(symbol_file.c_str());

  if (!success) {
    fwprintf(stderr, L"Symbol file upload failed\n");
    return 1;
  }

  wprintf(L"Uploaded symbols for windows-%s/%s/%s (%s %s)\n",
          pdb_info.cpu.c_str(), pdb_info.debug_file.c_str(),
          pdb_info.debug_identifier.c_str(), code_file.c_str(),
          file_version.c_str());
  return 0;
}
