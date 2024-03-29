






























#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <map>
#include <queue>
#include <vector>
#include <memory>

#include "common/dwarf/functioninfo.h"
#include "common/dwarf/bytereader.h"
#include "common/using_std_string.h"

namespace dwarf2reader {

CULineInfoHandler::CULineInfoHandler(std::vector<SourceFileInfo>* files,
                                     std::vector<string>* dirs,
                                     LineMap* linemap):linemap_(linemap),
                                                       files_(files),
                                                       dirs_(dirs) {
  
  
  assert(dirs->size() == 0);
  assert(files->size() == 0);
  dirs->push_back("");
  SourceFileInfo s;
  s.name = "";
  s.lowpc = ULLONG_MAX;
  files->push_back(s);
}

void CULineInfoHandler::DefineDir(const string& name, uint32 dir_num) {
  
  assert(dir_num == dirs_->size());
  dirs_->push_back(name);
}

void CULineInfoHandler::DefineFile(const string& name,
                                   int32 file_num, uint32 dir_num,
                                   uint64 mod_time, uint64 length) {
  assert(dir_num >= 0);
  assert(dir_num < dirs_->size());

  
  if (file_num == (int32)files_->size() || file_num == -1) {
    string dir = dirs_->at(dir_num);

    SourceFileInfo s;
    s.lowpc = ULLONG_MAX;

    if (dir == "") {
      s.name = name;
    } else {
      s.name = dir + "/" + name;
    }

    files_->push_back(s);
  } else {
    fprintf(stderr, "error in DefineFile");
  }
}

void CULineInfoHandler::AddLine(uint64 address, uint64 length, uint32 file_num,
                                uint32 line_num, uint32 column_num) {
  if (file_num < files_->size()) {
    linemap_->insert(
        std::make_pair(address,
                       std::make_pair(files_->at(file_num).name.c_str(),
                                      line_num)));

    if(address < files_->at(file_num).lowpc) {
      files_->at(file_num).lowpc = address;
    }
  } else {
    fprintf(stderr,"error in AddLine");
  }
}

bool CUFunctionInfoHandler::StartCompilationUnit(uint64 offset,
                                                 uint8 address_size,
                                                 uint8 offset_size,
                                                 uint64 cu_length,
                                                 uint8 dwarf_version) {
  current_compilation_unit_offset_ = offset;
  return true;
}






bool CUFunctionInfoHandler::StartDIE(uint64 offset, enum DwarfTag tag) {
  switch (tag) {
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine: {
      current_function_info_ = new FunctionInfo;
      current_function_info_->lowpc = current_function_info_->highpc = 0;
      current_function_info_->name = "";
      current_function_info_->line = 0;
      current_function_info_->file = "";
      offset_to_funcinfo_->insert(std::make_pair(offset,
                                                 current_function_info_));
    };
      
    case DW_TAG_compile_unit:
      return true;
    default:
      return false;
  }
  return false;
}



void CUFunctionInfoHandler::ProcessAttributeString(uint64 offset,
                                                   enum DwarfAttribute attr,
                                                   enum DwarfForm form,
                                                   const string &data) {
  if (current_function_info_) {
    if (attr == DW_AT_name)
      current_function_info_->name = data;
    else if(attr == DW_AT_MIPS_linkage_name)
      current_function_info_->mangled_name = data;
  }
}

void CUFunctionInfoHandler::ProcessAttributeUnsigned(uint64 offset,
                                                     enum DwarfAttribute attr,
                                                     enum DwarfForm form,
                                                     uint64 data) {
  if (attr == DW_AT_stmt_list) {
    SectionMap::const_iterator iter = sections_.find("__debug_line");
    assert(iter != sections_.end());

    
    std::auto_ptr<LineInfo> lireader(new LineInfo(iter->second.first + data,
                                                  iter->second.second  - data,
                                                  reader_, linehandler_));
    lireader->Start();
  } else if (current_function_info_) {
    switch (attr) {
      case DW_AT_low_pc:
        current_function_info_->lowpc = data;
        break;
      case DW_AT_high_pc:
        current_function_info_->highpc = data;
        break;
      case DW_AT_decl_line:
        current_function_info_->line = data;
        break;
      case DW_AT_decl_file:
        current_function_info_->file = files_->at(data).name;
        break;
      default:
        break;
    }
  }
}

void CUFunctionInfoHandler::ProcessAttributeReference(uint64 offset,
                                                      enum DwarfAttribute attr,
                                                      enum DwarfForm form,
                                                      uint64 data) {
  if (current_function_info_) {
    switch (attr) {
      case DW_AT_specification: {
        
        
        
        
        
        
        FunctionMap::iterator iter = offset_to_funcinfo_->find(data);
        if (iter != offset_to_funcinfo_->end()) {
          current_function_info_->name = iter->second->name;
          current_function_info_->mangled_name = iter->second->mangled_name;
        } else {
          
          fprintf(stderr, "Error: DW_AT_specification was seen before the referenced DIE! (Looking for DIE at offset %08llx, in DIE at offset %08llx)\n", data, offset);
        }
        break;
      }
      default:
        break;
    }
  }
}

void CUFunctionInfoHandler::EndDIE(uint64 offset) {
  if (current_function_info_ && current_function_info_->lowpc)
    address_to_funcinfo_->insert(std::make_pair(current_function_info_->lowpc,
                                                current_function_info_));
}

}  
