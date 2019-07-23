































#ifndef _PDB_SOURCE_LINE_WRITER_H__
#define _PDB_SOURCE_LINE_WRITER_H__

#include <atlcomcli.h>

#include <hash_map>
#include <string>

struct IDiaEnumLineNumbers;
struct IDiaSession;
struct IDiaSymbol;

namespace google_breakpad {

using std::wstring;
using stdext::hash_map;


struct PDBModuleInfo {
 public:
  
  wstring debug_file;

  
  
  
  
  
  
  
  wstring debug_identifier;

  
  
  wstring cpu;
};

class PDBSourceLineWriter {
 public:
  enum FileFormat {
    PDB_FILE,  
    EXE_FILE,  
    ANY_FILE   
  };

  explicit PDBSourceLineWriter();
  ~PDBSourceLineWriter();

  
  
  
  
  bool Open(const wstring &file, FileFormat format);

  
  
  
  bool OpenExecutable(const wstring &exe_file);

  
  
  bool WriteMap(FILE *map_file);

  
  void Close();

  
  
  bool GetModuleInfo(PDBModuleInfo *info);

  
  
  
  
  
  bool UsesGUID(bool *uses_guid);

 private:
  
  
  bool PrintLines(IDiaEnumLineNumbers *lines);

  
  
  
  
  
  bool PrintFunction(IDiaSymbol *function, IDiaSymbol *block);

  
  bool PrintFunctions();

  
  
  bool PrintSourceFiles();

  
  
  bool PrintFrameData();

  
  
  
  bool PrintCodePublicSymbol(IDiaSymbol *symbol);

  
  
  bool PrintPDBInfo();

  
  
  bool FileIDIsCached(const wstring &file) {
    return unique_files_.find(file) != unique_files_.end();
  };

  
  void CacheFileID(const wstring &file, DWORD id) {
    unique_files_[file] = id;
  };

  
  void StoreDuplicateFileID(const wstring &file, DWORD id) {
    hash_map<wstring, DWORD>::iterator iter = unique_files_.find(file);
    if (iter != unique_files_.end()) {
      
      file_ids_[id] = iter->second;
    }
  };

  
  
  
  
  DWORD GetRealFileID(DWORD id) {
    hash_map<DWORD, DWORD>::iterator iter = file_ids_.find(id);
    if (iter == file_ids_.end())
      return id;
    return iter->second;
  };

  
  
  
  
  
  static bool GetSymbolFunctionName(IDiaSymbol *function, BSTR *name,
                                    int *stack_param_size);

  
  
  
  static int GetFunctionStackParamSize(IDiaSymbol *function);

  
  CComPtr<IDiaSession> session_;

  
  FILE *output_;

  
  
  
  hash_map<DWORD, DWORD> file_ids_;
  
  hash_map<wstring, DWORD> unique_files_;

  
  PDBSourceLineWriter(const PDBSourceLineWriter&);
  void operator=(const PDBSourceLineWriter&);
};

}  

#endif
