




































#ifndef COMMON_LINUX_DWARF_LINE_TO_MODULE_H
#define COMMON_LINUX_DWARF_LINE_TO_MODULE_H

#include "common/module.h"
#include "common/dwarf/dwarf2reader.h"

namespace google_breakpad {


































































class DwarfLineToModule: public dwarf2reader::LineInfoHandler {
 public:
  
  
  
  
  
  
  
  DwarfLineToModule(Module *module, vector<Module::Line> *lines)
      : module_(module),
        lines_(lines),
        highest_file_number_(-1),
        omitted_line_end_(0),
        warned_bad_file_number_(false),
        warned_bad_directory_number_(false) { }
  
  ~DwarfLineToModule() { }

  void DefineDir(const std::string &name, uint32 dir_num);
  void DefineFile(const std::string &name, int32 file_num,
                  uint32 dir_num, uint64 mod_time,
                  uint64 length);
  void AddLine(uint64 address, uint64 length,
               uint32 file_num, uint32 line_num, uint32 column_num);

 private:

  typedef std::map<uint32, std::string> DirectoryTable;
  typedef std::map<uint32, Module::File *> FileTable;

  
  
  Module *module_;

  
  
  
  
  
  
  
  
  
  vector<Module::Line> *lines_;

  
  DirectoryTable directories_;

  
  FileTable files_;

  
  
  int32 highest_file_number_;
  
  
  
  
  uint64 omitted_line_end_;

  
  bool warned_bad_file_number_; 
  bool warned_bad_directory_number_; 
};

} 

#endif 
