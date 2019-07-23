


































#ifndef COMMON_MAC_DWARF_DWARF2READER_H__
#define COMMON_MAC_DWARF_DWARF2READER_H__

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/mac/dwarf/dwarf2enums.h"
#include "common/mac/dwarf/types.h"

using namespace std;

namespace dwarf2reader {
struct LineStateMachine;
class ByteReader;
class Dwarf2Handler;
class LineInfoHandler;



typedef map<string, pair<const char*, uint64> > SectionMap;
typedef list<pair<enum DwarfAttribute, enum DwarfForm> > AttributeList;
typedef AttributeList::iterator AttributeIterator;
typedef AttributeList::const_iterator ConstAttributeIterator;

struct LineInfoHeader {
  uint64 total_length;
  uint16 version;
  uint64 prologue_length;
  uint8 min_insn_length; 
  bool default_is_stmt; 
  int8 line_base;
  uint8 line_range;
  uint8 opcode_base;
  
  
  vector<unsigned char> *std_opcode_lengths;
};

class LineInfo {
 public:

  
  
  
  
  LineInfo(const char* buffer_, uint64 buffer_length,
           ByteReader* reader, LineInfoHandler* handler);

  virtual ~LineInfo() {
    if (header_.std_opcode_lengths) {
      delete header_.std_opcode_lengths;
    }
  }

  
  
  
  uint64 Start();

  
  
  
  
  
  
  
  
  
  static bool ProcessOneOpcode(ByteReader* reader,
                               LineInfoHandler* handler,
                               const struct LineInfoHeader &header,
                               const char* start,
                               struct LineStateMachine* lsm,
                               size_t* len,
                               uintptr_t pc,
                               bool *lsm_passes_pc);

 private:
  
  void ReadHeader();

  
  void ReadLines();

  
  LineInfoHandler* handler_;

  
  ByteReader* reader_;

  
  
  

  struct LineInfoHeader header_;

  
  
  
  const char* buffer_;
  uint64 buffer_length_;
  const char* after_header_;
};






class LineInfoHandler {
 public:
  LineInfoHandler() { }

  virtual ~LineInfoHandler() { }

  
  
  virtual void DefineDir(const string& name, uint32 dir_num) { }

  
  
  
  
  
  
  
  virtual void DefineFile(const string& name, int32 file_num,
                          uint32 dir_num, uint64 mod_time,
                          uint64 length) { }

  
  
  
  
  
  virtual void AddLine(uint64 address, uint32 file_num, uint32 line_num,
                       uint32 column_num) { }
};




































class CompilationUnit {
 public:

  
  
  
  CompilationUnit(const SectionMap& sections, uint64 offset,
                  ByteReader* reader, Dwarf2Handler* handler);
  virtual ~CompilationUnit() {
    if (abbrevs_) delete abbrevs_;
  }

  
  
  
  
  uint64 Start();

 private:

  
  
  
  struct Abbrev {
    uint32 number;
    enum DwarfTag tag;
    bool has_children;
    AttributeList attributes;
  };

  
  
  
  struct CompilationUnitHeader {
    uint64 length;
    uint16 version;
    uint64 abbrev_offset;
    uint8 address_size;
  } header_;

  
  void ReadHeader();

  
  void ReadAbbrevs();

  
  
  const char* ProcessDIE(uint64 dieoffset,
                                  const char* start,
                                  const Abbrev& abbrev);

  
  
  const char* ProcessAttribute(uint64 dieoffset,
                                        const char* start,
                                        enum DwarfAttribute attr,
                                        enum DwarfForm form);

  
  void ProcessDIEs();

  
  
  const char* SkipDIE(const char* start,
                               const Abbrev& abbrev);

  
  
  const char* SkipAttribute(const char* start,
                                     enum DwarfForm form);

  
  
  uint64 offset_from_section_start_;

  
  
  
  const char* buffer_;
  uint64 buffer_length_;
  const char* after_header_;

  
  ByteReader* reader_;

  
  const SectionMap& sections_;

  
  Dwarf2Handler* handler_;

  
  
  
  vector<Abbrev>* abbrevs_;

  
  
  
  const char* string_buffer_;
  uint64 string_buffer_length_;
};






class Dwarf2Handler {
 public:
  Dwarf2Handler() { }

  virtual ~Dwarf2Handler() { }

  
  
  
  virtual bool StartCompilationUnit(uint64 offset, uint8 address_size,
                                    uint8 offset_size, uint64 cu_length,
                                    uint8 dwarf_version) { return false; }

  
  
  
  virtual bool StartDIE(uint64 offset, enum DwarfTag tag,
                        const AttributeList& attrs) { return false; }

  
  
  
  
  virtual void ProcessAttributeUnsigned(uint64 offset,
                                        enum DwarfAttribute attr,
                                        enum DwarfForm form,
                                        uint64 data) { }

  
  
  
  
  virtual void ProcessAttributeSigned(uint64 offset,
                                      enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      int64 data) { }

  
  
  
  
  
  
  
  virtual void ProcessAttributeBuffer(uint64 offset,
                                      enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const char* data,
                                      uint64 len) { }

  
  
  
  
  virtual void ProcessAttributeString(uint64 offset,
                                      enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const string& data) { }

  
  
  
  
  virtual void EndDIE(uint64 offset) { }

};


}  

#endif  
