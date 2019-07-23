
































#ifndef COMMON_MAC_DWARF_FUNCTIONINFO_H__
#define COMMON_MAC_DWARF_FUNCTIONINFO_H__

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/mac/dwarf/dwarf2reader.h"


namespace dwarf2reader {

struct FunctionInfo {
  
  string name;
  
  string mangled_name;
  
  string file;
  
  uint32 line;
  
  uint64 lowpc;
  
  uint64 highpc;
};

struct SourceFileInfo {
  
  string name;
  
  uint64 lowpc;
};

typedef map<uint64, FunctionInfo*> FunctionMap;
typedef map<uint64, pair<string, uint32> > LineMap;




class CULineInfoHandler: public LineInfoHandler {
 public:

  
  CULineInfoHandler(vector<SourceFileInfo>* files,
                    vector<string>* dirs,
                    LineMap* linemap);
  virtual ~CULineInfoHandler() { }

  
  
  virtual void DefineDir(const string& name, uint32 dir_num);

  
  
  virtual void DefineFile(const string& name, int32 file_num,
                          uint32 dir_num, uint64 mod_time, uint64 length);


  
  
  
  
  
  virtual void AddLine(uint64 address, uint32 file_num, uint32 line_num,
                       uint32 column_num);


 private:
  LineMap* linemap_;
  vector<SourceFileInfo>* files_;
  vector<string>* dirs_;
};

class CUFunctionInfoHandler: public Dwarf2Handler {
 public:
  CUFunctionInfoHandler(vector<SourceFileInfo>* files,
                        vector<string>* dirs,
                        LineMap* linemap,
                        FunctionMap* offset_to_funcinfo,
                        FunctionMap* address_to_funcinfo,
                        CULineInfoHandler* linehandler,
                        const SectionMap& sections,
                        ByteReader* reader)
      : files_(files), dirs_(dirs), linemap_(linemap),
        offset_to_funcinfo_(offset_to_funcinfo),
        address_to_funcinfo_(address_to_funcinfo),
        linehandler_(linehandler), sections_(sections),
        reader_(reader), current_function_info_(NULL) { }

  virtual ~CUFunctionInfoHandler() { }

  
  
  

  virtual bool StartCompilationUnit(uint64 offset, uint8 address_size,
                                    uint8 offset_size, uint64 cu_length,
                                    uint8 dwarf_version);

  
  
  virtual bool StartDIE(uint64 offset, enum DwarfTag tag,
                        const AttributeList& attrs);

  
  
  
  
  virtual void ProcessAttributeUnsigned(uint64 offset,
                                        enum DwarfAttribute attr,
                                        enum DwarfForm form,
                                        uint64 data);

  
  
  
  
  virtual void ProcessAttributeString(uint64 offset,
                                      enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const string& data);

  
  
  
  
  virtual void EndDIE(uint64 offset);

 private:
  vector<SourceFileInfo>* files_;
  vector<string>* dirs_;
  LineMap* linemap_;
  FunctionMap* offset_to_funcinfo_;
  FunctionMap* address_to_funcinfo_;
  CULineInfoHandler* linehandler_;
  const SectionMap& sections_;
  ByteReader* reader_;
  FunctionInfo* current_function_info_;
  uint64 current_compilation_unit_offset_;
};

}  
#endif  
