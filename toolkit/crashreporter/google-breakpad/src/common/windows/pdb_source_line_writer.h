































#ifndef _PDB_SOURCE_LINE_WRITER_H__
#define _PDB_SOURCE_LINE_WRITER_H__

#include <atlcomcli.h>

#include <string>

struct IDiaEnumLineNumbers;
struct IDiaSession;
struct IDiaSymbol;

namespace google_breakpad {

using std::wstring;


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

  
  
  bool PrintFunction(IDiaSymbol *function);

  
  bool PrintFunctions();

  
  
  bool PrintSourceFiles();

  
  
  bool PrintFrameData();

  
  
  
  bool PrintCodePublicSymbol(IDiaSymbol *symbol);

  
  
  bool PrintPDBInfo();

  
  
  
  
  
  static bool GetSymbolFunctionName(IDiaSymbol *function, BSTR *name,
                                    int *stack_param_size);

  
  
  
  static int GetFunctionStackParamSize(IDiaSymbol *function);

  
  CComPtr<IDiaSession> session_;

  
  FILE *output_;

  
  PDBSourceLineWriter(const PDBSourceLineWriter&);
  void operator=(const PDBSourceLineWriter&);
};

}  

#endif
