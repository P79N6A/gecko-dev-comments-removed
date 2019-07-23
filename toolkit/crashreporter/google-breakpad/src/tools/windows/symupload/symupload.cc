






































#include <Windows.h>
#include <DbgHelp.h>
#include <WinInet.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "common/windows/http_upload.h"
#include "common/windows/pdb_source_line_writer.h"

using std::string;
using std::wstring;
using std::vector;
using std::map;
using google_airbag::HTTPUpload;
using google_airbag::PDBSourceLineWriter;



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
  _snwprintf_s(ver_string, sizeof(ver_string) / sizeof(wchar_t), _TRUNCATE,
               L"%d.%d.%d.%d",
               file_info->dwFileVersionMS >> 16,
               file_info->dwFileVersionMS & 0xffff,
               file_info->dwFileVersionLS >> 16,
               file_info->dwFileVersionLS & 0xffff);
  *version = ver_string;
  return true;
}




static bool DumpSymbolsToTempFile(const wchar_t *exe_file,
                                  wstring *temp_file_path,
                                  wstring *module_guid) {
  google_airbag::PDBSourceLineWriter writer;
  if (!writer.Open(exe_file, PDBSourceLineWriter::EXE_FILE)) {
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
  if (_wfopen_s(&temp_file, temp_filename, L"w") != 0) {
    return false;
  }

  bool success = writer.WriteMap(temp_file);
  fclose(temp_file);
  if (!success) {
    _wunlink(temp_filename);
    return false;
  }

  *temp_file_path = temp_filename;
  *module_guid = writer.GetModuleGUID();
  return true;
}


static wstring GetBaseName(const wstring &filename) {
  wstring base_name(filename);
  size_t slash_pos = base_name.find_last_of(L"/\\");
  if (slash_pos != string::npos) {
    base_name.erase(0, slash_pos + 1);
  }
  return base_name;
}

int wmain(int argc, wchar_t *argv[]) {
  if (argc < 3) {
    wprintf(L"Usage: %s file.[exe|dll] <symbol upload URL>\n", argv[0]);
    return 0;
  }
  const wchar_t *module = argv[1], *url = argv[2];
  wstring module_basename = GetBaseName(module);

  wstring symbol_file, module_guid;
  if (!DumpSymbolsToTempFile(module, &symbol_file, &module_guid)) {
    fwprintf(stderr, L"Could not get symbol data from %s\n", module);
    return 1;
  }

  map<wstring, wstring> parameters;
  parameters[L"module"] = module_basename;
  parameters[L"guid"] = module_guid;

  
  
  wstring file_version;
  if (GetFileVersionString(module, &file_version)) {
    parameters[L"version"] = file_version;
  } else {
    fwprintf(stderr, L"Warning: Could not get file version for %s\n", module);
  }

  bool success = HTTPUpload::SendRequest(url, parameters,
                                         symbol_file, L"symbol_file");
  _wunlink(symbol_file.c_str());

  if (!success) {
    fwprintf(stderr, L"Symbol file upload failed\n");
    return 1;
  }

  wprintf(L"Uploaded symbols for %s/%s/%s\n",
         module_basename.c_str(), file_version.c_str(), module_guid.c_str());
  return 0;
}
