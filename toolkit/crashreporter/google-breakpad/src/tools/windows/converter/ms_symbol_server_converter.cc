



































#include <windows.h>
#include <dbghelp.h>

#include <cassert>
#include <cstdio>

#include "tools/windows/converter/ms_symbol_server_converter.h"
#include "common/windows/pdb_source_line_writer.h"
#include "common/windows/string_utils-inl.h"





#ifndef SYMOPT_NO_PROMPTS
#define SYMOPT_NO_PROMPTS 0x00080000
#endif  

namespace google_breakpad {





#if _MSC_VER >= 1400  
#define SSCANF sscanf_s
#else  
#define SSCANF sscanf
#endif  

bool GUIDOrSignatureIdentifier::InitializeFromString(
    const string &identifier) {
  type_ = TYPE_NONE;

  size_t length = identifier.length();

  if (length > 32 && length <= 40) {
    
    if (SSCANF(identifier.c_str(),
               "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%X",
               &guid_.Data1, &guid_.Data2, &guid_.Data3,
               &guid_.Data4[0], &guid_.Data4[1],
               &guid_.Data4[2], &guid_.Data4[3],
               &guid_.Data4[4], &guid_.Data4[5],
               &guid_.Data4[6], &guid_.Data4[7],
               &age_) != 12) {
      return false;
    }

    type_ = TYPE_GUID;
  } else if (length > 8 && length <= 15) {
    
    if (SSCANF(identifier.c_str(), "%08X%x", &signature_, &age_) != 2) {
      return false;
    }

    type_ = TYPE_SIGNATURE;
  } else {
    return false;
  }

  return true;
}

#undef SSCANF

MSSymbolServerConverter::MSSymbolServerConverter(
    const string &local_cache, const vector<string> &symbol_servers)
    : symbol_path_(),
      fail_dns_(false),
      fail_timeout_(false),
      fail_not_found_(false) {
  
  
  
  

  assert(symbol_servers.size() > 0);

  
  
  const char *kInvalidCharacters = "*;";
  assert(local_cache.find_first_of(kInvalidCharacters) == string::npos);

  for (vector<string>::const_iterator symbol_server = symbol_servers.begin();
       symbol_server != symbol_servers.end();
       ++symbol_server) {
    
    
    
    
    
    
    
    
    
    
    
    

    assert((*symbol_server).find_first_of(kInvalidCharacters) == string::npos);
    symbol_path_ += "srv*" + local_cache + "*" + *symbol_server + ";";
  }

  
  symbol_path_.erase(symbol_path_.length() - 1);
}


class AutoSymSrv {
 public:
  AutoSymSrv() : initialized_(false) {}

  ~AutoSymSrv() {
    if (!Cleanup()) {
      
      
      fprintf(stderr, "~AutoSymSrv: SymCleanup: error %d\n", GetLastError());
    }
  }

  bool Initialize(HANDLE process, char *path, bool invade_process) {
    process_ = process;
    initialized_ = SymInitialize(process, path, invade_process) == TRUE;
    return initialized_;
  }

  bool Cleanup() {
    if (initialized_) {
      if (SymCleanup(process_)) {
        initialized_ = false;
        return true;
      }
      return false;
    }

    return true;
  }

 private:
  HANDLE process_;
  bool initialized_;
};




class AutoDeleter {
 public:
  AutoDeleter(const string &path) : path_(path) {}

  ~AutoDeleter() {
    int error;
    if ((error = Delete()) != 0) {
      
      
      fprintf(stderr, "~AutoDeleter: Delete: error %d for %s\n",
              error, path_.c_str());
    }
  }

  int Delete() {
    if (path_.empty())
      return 0;

    int error = remove(path_.c_str());
    Release();
    return error;
  }

  void Release() {
    path_.clear();
  }

 private:
  string path_;
};

MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocateSymbolFile(const MissingSymbolInfo &missing,
                                          string *symbol_file) {
  assert(symbol_file);
  symbol_file->clear();

  GUIDOrSignatureIdentifier identifier;
  if (!identifier.InitializeFromString(missing.debug_identifier)) {
    fprintf(stderr,
            "LocateSymbolFile: Unparseable debug_identifier for %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str());
    return LOCATE_FAILURE;
  }

  HANDLE process = GetCurrentProcess();  
  AutoSymSrv symsrv;
  if (!symsrv.Initialize(process,
                         const_cast<char *>(symbol_path_.c_str()),
                         false)) {
    fprintf(stderr, "LocateSymbolFile: SymInitialize: error %d for %s %s %s\n",
            GetLastError(),
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str());
    return LOCATE_FAILURE;
  }

  if (!SymRegisterCallback64(process, SymCallback,
                             reinterpret_cast<ULONG64>(this))) {
    fprintf(stderr,
            "LocateSymbolFile: SymRegisterCallback64: error %d for %s %s %s\n",
            GetLastError(),
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str());
    return LOCATE_FAILURE;
  }

  
  
  DWORD options = SymGetOptions() | SYMOPT_DEBUG | SYMOPT_NO_PROMPTS |
                  SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_SECURE;
  SymSetOptions(options);

  
  fail_dns_ = false;
  fail_timeout_ = false;
  fail_not_found_ = false;

  
  char path[MAX_PATH];
  if (!SymFindFileInPath(
          process, NULL,
          const_cast<char *>(missing.debug_file.c_str()),
          const_cast<void *>(identifier.guid_or_signature_pointer()),
          identifier.age(), 0,
          identifier.type() == GUIDOrSignatureIdentifier::TYPE_GUID ?
              SSRVOPT_GUIDPTR : SSRVOPT_DWORDPTR,
          path, SymFindFileInPathCallback, this)) {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND) {
      
      

      
      if (fail_dns_ || fail_timeout_) {
        return LOCATE_RETRY;
      }

      
      if (fail_not_found_) {
        fprintf(stderr,
                "LocateSymbolFile: SymFindFileInPath: LOCATE_NOT_FOUND error "
                "for %s %s %s\n",
                missing.debug_file.c_str(),
                missing.debug_identifier.c_str(),
                missing.version.c_str());
        return LOCATE_NOT_FOUND;
      }

      
      
    }

    fprintf(stderr,
            "LocateSymbolFile: SymFindFileInPath: error %d for %s %s %s\n",
            error,
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str());
    return LOCATE_FAILURE;
  }

  
  
  AutoDeleter deleter(path);

  
  
  if (!symsrv.Cleanup()) {
    fprintf(stderr, "LocateSymbolFile: SymCleanup: error %d for %s %s %s\n",
            GetLastError(),
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str());
    return LOCATE_FAILURE;
  }

  deleter.Release();

  *symbol_file = path;
  return LOCATE_SUCCESS;
}


BOOL CALLBACK MSSymbolServerConverter::SymCallback(HANDLE process,
                                                   ULONG action,
                                                   ULONG64 data,
                                                   ULONG64 context) {
  MSSymbolServerConverter *self =
      reinterpret_cast<MSSymbolServerConverter *>(context);

  switch (action) {
    case CBA_EVENT: {
      IMAGEHLP_CBA_EVENT *cba_event =
          reinterpret_cast<IMAGEHLP_CBA_EVENT *>(data);

      
      
      
      
      string desc(cba_event->desc);

      
      
      struct desc_action {
        const char *desc;  
        bool *action;      
      };

      static const desc_action desc_actions[] = {
        
        
        { "SYMSRV:  The server name or address could not be resolved\n",
          &self->fail_dns_ },

        
        { "SYMSRV:  A connection with the server could not be established\n",
          &self->fail_timeout_ },

        
        
        { "SYMSRV:  The operation timed out\n",
          &self->fail_timeout_ },

        
        
        
        
        
        
        { " not found\n",
          &self->fail_not_found_ }
      };

      for (int desc_action_index = 0;
           desc_action_index < sizeof(desc_actions) / sizeof(desc_action);
           ++desc_action_index) {
        if (desc.find(desc_actions[desc_action_index].desc) != string::npos) {
          *(desc_actions[desc_action_index].action) = true;
          break;
        }
      }

      break;
    }
  }

  
  return FALSE;
}


BOOL CALLBACK MSSymbolServerConverter::SymFindFileInPathCallback(
    char *filename, void *context) {
  
  
  return FALSE;
}

MSSymbolServerConverter::LocateResult
MSSymbolServerConverter::LocateAndConvertSymbolFile(
    const MissingSymbolInfo &missing,
    bool keep_symbol_file,
    string *converted_symbol_file,
    string *symbol_file) {
  assert(converted_symbol_file);
  converted_symbol_file->clear();
  if (symbol_file) {
    symbol_file->clear();
  }

  string pdb_file;
  LocateResult result = LocateSymbolFile(missing, &pdb_file);
  if (result != LOCATE_SUCCESS) {
    return result;
  }

  if (symbol_file && keep_symbol_file) {
    *symbol_file = pdb_file;
  }

  
  
  
  
  AutoDeleter pdb_deleter(pdb_file);

  
  
  string pdb_extension = pdb_file.substr(pdb_file.length() - 4);
  
  if (_stricmp(pdb_extension.c_str(), ".pdb") != 0) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "LocateSymbolFile: no .pdb extension for %s %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file.c_str());
    return LOCATE_FAILURE;
  }

  
  wstring pdb_file_w;
  if (!WindowsStringUtils::safe_mbstowcs(pdb_file, &pdb_file_w)) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "WindowsStringUtils::safe_mbstowcs failed for %s %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file.c_str());
    return LOCATE_FAILURE;
  }

  PDBSourceLineWriter writer;
  if (!writer.Open(pdb_file_w, PDBSourceLineWriter::PDB_FILE)) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "PDBSourceLineWriter::Open failed for %s %s %s %ws\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file_w.c_str());
    return LOCATE_FAILURE;
  }

  *converted_symbol_file = pdb_file.substr(0, pdb_file.length() - 4) + ".sym";

  FILE *converted_output = NULL;
#if _MSC_VER >= 1400  
  errno_t err;
  if ((err = fopen_s(&converted_output, converted_symbol_file->c_str(), "w"))
      != 0) {
#else




  int err;
  if (!(converted_output = fopen(converted_symbol_file->c_str(), "w"))) {
    err = -1;
#endif
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "fopen_s: error %d for %s %s %s %s\n",
            err,
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            converted_symbol_file->c_str());
    return LOCATE_FAILURE;
  }

  AutoDeleter sym_deleter(*converted_symbol_file);

  bool success = writer.WriteMap(converted_output);
  fclose(converted_output);

  if (!success) {
    fprintf(stderr, "LocateAndConvertSymbolFile: "
            "PDBSourceLineWriter::WriteMap failed for %s %s %s %s\n",
            missing.debug_file.c_str(),
            missing.debug_identifier.c_str(),
            missing.version.c_str(),
            pdb_file.c_str());
    return LOCATE_FAILURE;
  }

  if (keep_symbol_file) {
    pdb_deleter.Release();
  }

  sym_deleter.Release();

  return LOCATE_SUCCESS;
}

}  
