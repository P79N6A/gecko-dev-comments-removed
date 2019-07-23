
































































#ifndef MS_SYMBOL_SERVER_CONVERTER_H__
#define MS_SYMBOL_SERVER_CONVERTER_H__

#include <Windows.h>

#include <string>
#include <vector>

namespace google_breakpad {

using std::string;
using std::vector;





struct MissingSymbolInfo {
  string code_file;
  string code_identifier;
  string debug_file;
  string debug_identifier;
  string version;
};

class GUIDOrSignatureIdentifier {
 public:
  enum GUIDOrSignatureType {
    TYPE_NONE = 0,
    TYPE_GUID,
    TYPE_SIGNATURE
  };

  GUIDOrSignatureIdentifier() : type_(TYPE_NONE) {}

  
  
  
  
  bool InitializeFromString(const string &identifier);

  GUIDOrSignatureType type() const { return type_; }
  GUID guid() const { return guid_; }
  DWORD signature() const { return signature_; }
  int age() const { return age_; }
  const void *guid_or_signature_pointer() const { return &guid_; }

 private:
  GUIDOrSignatureType type_;

  
  union {
    GUID guid_;
    DWORD signature_;
  };

  
  
  int age_;
};

class MSSymbolServerConverter {
 public:
  enum LocateResult {
    LOCATE_FAILURE = 0,
    LOCATE_NOT_FOUND,    
    LOCATE_RETRY,        
    LOCATE_SUCCESS
  };

  
  
  
  
  
  
  
  
  
  MSSymbolServerConverter(const string &local_cache,
                          const vector<string> &symbol_servers);

  
  
  
  
  
  LocateResult LocateSymbolFile(const MissingSymbolInfo &missing,
                                string *symbol_file);

  
  
  
  
  
  
  
  
  
  
  LocateResult LocateAndConvertSymbolFile(const MissingSymbolInfo &missing,
                                          bool keep_symbol_file,
                                          string *converted_symbol_file,
                                          string *symbol_file);

 private:
  
  
  
  
  static BOOL CALLBACK SymCallback(HANDLE process, ULONG action, ULONG64 data,
                                   ULONG64 context);

  
  
  
  
  
  static BOOL CALLBACK SymFindFileInPathCallback(char *filename,
                                                 void *context);

  
  
  string symbol_path_;

  
  
  bool fail_dns_;        
  bool fail_timeout_;    
  bool fail_not_found_;  
                         
};

}  

#endif
