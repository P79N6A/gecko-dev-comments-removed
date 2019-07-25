

































#include "common/dwarf_line_to_module.h"






static bool PathIsAbsolute(const string &path) {
  return (path.size() >= 1 && path[0] == '/');
}



static string ExpandPath(const string &path, const string &base) {
  if (PathIsAbsolute(path))
    return path;
  return base + "/" + path;
}

namespace google_breakpad {

void DwarfLineToModule::DefineDir(const string &name, uint32 dir_num) {
  
  
  if (dir_num != 0)
    directories_[dir_num] = name;
}

void DwarfLineToModule::DefineFile(const string &name, int32 file_num,
                                   uint32 dir_num, uint64 mod_time,
                                   uint64 length) {
  if (file_num == -1)
    file_num = ++highest_file_number_;
  else if (file_num > highest_file_number_)
    highest_file_number_ = file_num;

  std::string full_name;
  if (dir_num != 0) {
    DirectoryTable::const_iterator directory_it = directories_.find(dir_num);
    if (directory_it != directories_.end()) {
      full_name = ExpandPath(name, directory_it->second);
    } else {
      if (!warned_bad_directory_number_) {
        fprintf(stderr, "warning: DWARF line number data refers to undefined"
                " directory numbers\n");
        warned_bad_directory_number_ = true;
      }
      full_name = name; 
    }
  } else {
    
    
    full_name = name;
  }

  
  
  files_[file_num] = module_->FindFile(full_name);
}

void DwarfLineToModule::AddLine(uint64 address, uint64 length,
                                uint32 file_num, uint32 line_num,
                                uint32 column_num) {
  if (length == 0)
    return;

  
  if (address + length < address)
    length = -address;

  
  if (address == 0 || address == omitted_line_end_) {
    omitted_line_end_ = address + length;
    return;
  } else {
    omitted_line_end_ = 0;
  }

  
  Module::File *file = files_[file_num];
  if (!file) {
    if (!warned_bad_file_number_) {
      fprintf(stderr, "warning: DWARF line number data refers to "
              "undefined file numbers\n");
      warned_bad_file_number_ = true;
    }
    return;
  }
  Module::Line line;
  line.address = address;
  
  line.size = length;
  line.file = file;
  line.number = line_num;
  lines_->push_back(line);
}

} 
