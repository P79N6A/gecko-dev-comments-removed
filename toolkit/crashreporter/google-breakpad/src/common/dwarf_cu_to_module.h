





































#ifndef COMMON_LINUX_DWARF_CU_TO_MODULE_H__
#define COMMON_LINUX_DWARF_CU_TO_MODULE_H__

#include <string>

#include "common/language.h"
#include "common/module.h"
#include "common/dwarf/bytereader.h"
#include "common/dwarf/dwarf2diehandler.h"
#include "common/dwarf/dwarf2reader.h"
#include "common/using_std_string.h"

namespace google_breakpad {

using dwarf2reader::DwarfAttribute;
using dwarf2reader::DwarfForm;
using dwarf2reader::DwarfLanguage;
using dwarf2reader::DwarfTag;








class DwarfCUToModule: public dwarf2reader::RootDIEHandler {
  struct FilePrivate;
 public:

  
  
  
  
  
  
  
  
  struct FileContext {
    FileContext(const string &filename_arg, Module *module_arg);
    ~FileContext();

    
    string filename;

    
    
    dwarf2reader::SectionMap section_map;

    
    Module *module;

    
    FilePrivate *file_private;
  };

  
  
  
  
  class LineToModuleHandler {
   public:
    LineToModuleHandler() { }
    virtual ~LineToModuleHandler() { }

    
    
    
    
    virtual void StartCompilationUnit(const string& compilation_dir) = 0;

    
    
    
    
    virtual void ReadProgram(const char *program, uint64 length,
                             Module *module, vector<Module::Line> *lines) = 0;
  };

  
  
  
  
  class WarningReporter {
   public:
    
    
    WarningReporter(const string &filename, uint64 cu_offset)
        : filename_(filename), cu_offset_(cu_offset), printed_cu_header_(false),
          printed_unpaired_header_(false),
          uncovered_warnings_enabled_(false) { }
    virtual ~WarningReporter() { }

    
    virtual void SetCUName(const string &name) { cu_name_ = name; }

    
    
    
    
    virtual bool uncovered_warnings_enabled() const {
      return uncovered_warnings_enabled_;
    }
    virtual void set_uncovered_warnings_enabled(bool value) {
      uncovered_warnings_enabled_ = value;
    }

    
    
    
    virtual void UnknownSpecification(uint64 offset, uint64 target);

    
    
    virtual void UnknownAbstractOrigin(uint64 offset, uint64 target);

    
    virtual void MissingSection(const string &section_name);

    
    virtual void BadLineInfoOffset(uint64 offset);

    
    virtual void UncoveredFunction(const Module::Function &function);

    
    
    virtual void UncoveredLine(const Module::Line &line);

    
    
    
    virtual void UnnamedFunction(uint64 offset);

   protected:
    string filename_;
    uint64 cu_offset_;
    string cu_name_;
    bool printed_cu_header_;
    bool printed_unpaired_header_;
    bool uncovered_warnings_enabled_;

   private:
    
    void CUHeading();
    
    void UncoveredHeading();
  };

  
  
  
  
  
  
  DwarfCUToModule(FileContext *file_context,
                  LineToModuleHandler *line_reader,
                  WarningReporter *reporter);
  ~DwarfCUToModule();

  void ProcessAttributeSigned(enum DwarfAttribute attr,
                              enum DwarfForm form,
                              int64 data);
  void ProcessAttributeUnsigned(enum DwarfAttribute attr,
                                enum DwarfForm form,
                                uint64 data);
  void ProcessAttributeString(enum DwarfAttribute attr,
                              enum DwarfForm form,
                              const string &data);
  bool EndAttributes();
  DIEHandler *FindChildHandler(uint64 offset, enum DwarfTag tag);

  
  
  void Finish();

  bool StartCompilationUnit(uint64 offset, uint8 address_size,
                            uint8 offset_size, uint64 cu_length,
                            uint8 dwarf_version);
  bool StartRootDIE(uint64 offset, enum DwarfTag tag);

 private:

  
  
  struct Specification;
  struct CUContext;
  struct DIEContext;
  class GenericDIEHandler;
  class FuncHandler;
  class NamedScopeHandler;

  
  typedef map<uint64, Specification> SpecificationByOffset;

  
  void SetLanguage(DwarfLanguage language);
  
  
  
  
  
  void ReadSourceLines(uint64 offset);

  
  
  
  
  void AssignLinesToFunctions();

  
  
  
  
  

  
  LineToModuleHandler *line_reader_;

  
  CUContext *cu_context_;

  
  DIEContext *child_context_;

  
  bool has_source_line_info_;

  
  
  uint64 source_line_offset_;

  
  
  
  vector<Module::Line> lines_;
};

} 

#endif  
