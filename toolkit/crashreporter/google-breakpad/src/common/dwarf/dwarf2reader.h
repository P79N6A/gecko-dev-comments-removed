






































#ifndef COMMON_DWARF_DWARF2READER_H__
#define COMMON_DWARF_DWARF2READER_H__

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/dwarf/bytereader.h"
#include "common/dwarf/dwarf2enums.h"
#include "common/dwarf/types.h"

using namespace std;

namespace dwarf2reader {
struct LineStateMachine;
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
                               uintptr pc,
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

  
  
  
  
  
  
  virtual void AddLine(uint64 address, uint64 length,
                       uint32 file_num, uint32 line_num, uint32 column_num) { }
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
    uint64 number;
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

  
  
  
  
  
  virtual void ProcessAttributeReference(uint64 offset,
                                         enum DwarfAttribute attr,
                                         enum DwarfForm form,
                                         uint64 data) { }

  
  
  
  
  
  
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























































































































































class CallFrameInfo {
 public:
  
  
  enum EntryKind { kUnknown, kCIE, kFDE, kTerminator };

  
  
  class Handler;

  
  
  class Reporter;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CallFrameInfo(const char *buffer, size_t buffer_length,
                ByteReader *reader, Handler *handler, Reporter *reporter,
                bool eh_frame = false)
      : buffer_(buffer), buffer_length_(buffer_length),
        reader_(reader), handler_(handler), reporter_(reporter),
        eh_frame_(eh_frame) { }

  ~CallFrameInfo() { }

  
  
  
  bool Start();

  
  static const char *KindName(EntryKind kind);

 private:

  struct CIE;

  
  struct Entry {
    
    
    size_t offset;

    
    const char *start;
    
    
    
    
    
    
    EntryKind kind;

    
    
    const char *fields;

    
    const char *instructions;

    
    
    
    
    const char *end;

    
    
    uint64 id;

    
    
    CIE *cie;
  };

  
  struct CIE: public Entry {
    uint8 version;                      
    string augmentation;                
    uint64 code_alignment_factor;       
    int data_alignment_factor;          
    unsigned return_address_register;   

    
    bool has_z_augmentation;
 
    
    
    bool has_z_lsda;                    
    bool has_z_personality;             
    bool has_z_signal_frame;            

    
    
    DwarfPointerEncoding lsda_encoding;

    
    
    DwarfPointerEncoding personality_encoding;

    
    
    
    uint64 personality_address;

    
    
    
    
    DwarfPointerEncoding pointer_encoding;
  };

  
  struct FDE: public Entry {
    uint64 address;                     
    uint64 size;                        

    
    
    
    uint64 lsda_address;
  };

  
  class Rule;
  class UndefinedRule;
  class SameValueRule;
  class OffsetRule;
  class ValOffsetRule;
  class RegisterRule;
  class ExpressionRule;
  class ValExpressionRule;
  class RuleMap;
  class State;
  
  
  
  
  
  
  
  bool ReadEntryPrologue(const char *cursor, Entry *entry);

  
  
  
  
  
  bool ReadCIEFields(CIE *cie);

  
  
  
  
  
  
  bool ReadFDEFields(FDE *fde);

  
  
  
  bool ReportIncomplete(Entry *entry);

  
  static bool IsIndirectEncoding(DwarfPointerEncoding encoding) {
    return encoding & DW_EH_PE_indirect;
  }

  
  const char *buffer_;
  size_t buffer_length_;

  
  ByteReader *reader_;

  
  Handler *handler_;

  
  Reporter *reporter_;

  
  bool eh_frame_;
};



class CallFrameInfo::Handler {
 public:
  
  enum { kCFARegister = -1 };

  Handler() { }
  virtual ~Handler() { }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool Entry(size_t offset, uint64 address, uint64 length,
                     uint8 version, const string &augmentation,
                     unsigned return_address) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  virtual bool UndefinedRule(uint64 address, int reg) = 0;

  
  
  virtual bool SameValueRule(uint64 address, int reg) = 0;

  
  
  virtual bool OffsetRule(uint64 address, int reg,
                          int base_register, long offset) = 0;

  
  
  
  virtual bool ValOffsetRule(uint64 address, int reg,
                             int base_register, long offset) = 0;

  
  
  
  
  
  virtual bool RegisterRule(uint64 address, int reg, int base_register) = 0;

  
  
  virtual bool ExpressionRule(uint64 address, int reg,
                              const string &expression) = 0;

  
  
  
  virtual bool ValExpressionRule(uint64 address, int reg,
                                 const string &expression) = 0;

  
  
  
  
  virtual bool End() = 0;

  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  virtual bool PersonalityRoutine(uint64 address, bool indirect) {
    return true;
  }

  
  
  
  
  
  virtual bool LanguageSpecificDataArea(uint64 address, bool indirect) {
    return true;
  }

  
  
  
  
  
  
  
  
  virtual bool SignalHandler() { return true; }
};





class CallFrameInfo::Reporter {
 public:
  
  
  
  
  
  
  
  Reporter(const string &filename,
           const string &section = ".debug_frame")
      : filename_(filename), section_(section) { }
  virtual ~Reporter() { }

  
  
  
  virtual void Incomplete(uint64 offset, CallFrameInfo::EntryKind kind);

  
  
  
  
  virtual void EarlyEHTerminator(uint64 offset);

  
  
  virtual void CIEPointerOutOfRange(uint64 offset, uint64 cie_offset);

  
  
  virtual void BadCIEId(uint64 offset, uint64 cie_offset);

  
  
  
  virtual void UnrecognizedVersion(uint64 offset, int version);

  
  
  
  virtual void UnrecognizedAugmentation(uint64 offset,
                                        const string &augmentation);

  
  
  virtual void InvalidPointerEncoding(uint64 offset, uint8 encoding);

  
  
  virtual void UnusablePointerEncoding(uint64 offset, uint8 encoding);

  
  
  virtual void RestoreInCIE(uint64 offset, uint64 insn_offset);

  
  
  virtual void BadInstruction(uint64 offset, CallFrameInfo::EntryKind kind,
                              uint64 insn_offset);

  
  
  
  virtual void NoCFARule(uint64 offset, CallFrameInfo::EntryKind kind, 
                         uint64 insn_offset);

  
  
  
  virtual void EmptyStateStack(uint64 offset, CallFrameInfo::EntryKind kind, 
                               uint64 insn_offset);

  
  
  
  
  
  virtual void ClearingCFARule(uint64 offset, CallFrameInfo::EntryKind kind, 
                               uint64 insn_offset);

 protected:
  
  string filename_;

  
  string section_;
};

}  

#endif
