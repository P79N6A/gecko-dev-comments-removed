




























#include <stdio.h>
#include <string.h>

#include <map>
#include <utility>
#include <vector>

#include "processor/address_map-inl.h"
#include "processor/contained_range_map-inl.h"
#include "processor/range_map-inl.h"

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/stack_frame.h"
#include "processor/linked_ptr.h"
#include "processor/scoped_ptr.h"
#include "processor/stack_frame_info.h"

using std::map;
using std::vector;
using std::make_pair;
#ifndef BSLR_NO_HASH_MAP
using __gnu_cxx::hash;
#endif  

namespace google_breakpad {

struct BasicSourceLineResolver::Line {
  Line(MemAddr addr, MemAddr code_size, int file_id, int source_line)
      : address(addr)
      , size(code_size)
      , source_file_id(file_id)
      , line(source_line) { }

  MemAddr address;
  MemAddr size;
  int source_file_id;
  int line;
};

struct BasicSourceLineResolver::Function {
  Function(const string &function_name,
           MemAddr function_address,
           MemAddr code_size,
           int set_parameter_size)
      : name(function_name), address(function_address), size(code_size),
        parameter_size(set_parameter_size) { }

  string name;
  MemAddr address;
  MemAddr size;

  
  int parameter_size;

  RangeMap< MemAddr, linked_ptr<Line> > lines;
};

struct BasicSourceLineResolver::PublicSymbol {
  PublicSymbol(const string& set_name,
               MemAddr set_address,
               int set_parameter_size)
      : name(set_name),
        address(set_address),
        parameter_size(set_parameter_size) {}

  string name;
  MemAddr address;

  
  
  
  int parameter_size;
};

class BasicSourceLineResolver::Module {
 public:
  Module(const string &name) : name_(name) { }

  
  bool LoadMap(const string &map_file);

  
  
  
  
  
  StackFrameInfo* LookupAddress(StackFrame *frame) const;

 private:
  friend class BasicSourceLineResolver;
#ifdef BSLR_NO_HASH_MAP
  typedef map<int, string> FileMap;
#else  
  typedef hash_map<int, string> FileMap;
#endif  

  
  
  
  
  enum StackInfoTypes {
    STACK_INFO_FPO = 0,
    STACK_INFO_TRAP,  
    STACK_INFO_TSS,   
    STACK_INFO_STANDARD,
    STACK_INFO_FRAME_DATA,
    STACK_INFO_LAST,  
    STACK_INFO_UNKNOWN = -1
  };

  
  
  
  
  
  
  
  
  
  
  static bool Tokenize(char *line, int max_tokens, vector<char*> *tokens);

  
  bool ParseFile(char *file_line);

  
  Function* ParseFunction(char *function_line);

  
  Line* ParseLine(char *line_line);

  
  
  bool ParsePublicSymbol(char *public_line);

  
  bool ParseStackInfo(char *stack_info_line);

  string name_;
  FileMap files_;
  RangeMap< MemAddr, linked_ptr<Function> > functions_;
  AddressMap< MemAddr, linked_ptr<PublicSymbol> > public_symbols_;

  
  
  
  
  ContainedRangeMap< MemAddr, linked_ptr<StackFrameInfo> >
      stack_info_[STACK_INFO_LAST];
};

BasicSourceLineResolver::BasicSourceLineResolver() : modules_(new ModuleMap) {
}

BasicSourceLineResolver::~BasicSourceLineResolver() {
  ModuleMap::iterator it;
  for (it = modules_->begin(); it != modules_->end(); ++it) {
    delete it->second;
  }
  delete modules_;
}

bool BasicSourceLineResolver::LoadModule(const string &module_name,
                                         const string &map_file) {
  
  if (modules_->find(module_name) != modules_->end()) {
    BPLOG(INFO) << "Symbols for module " << module_name << " already loaded";
    return false;
  }

  BPLOG(INFO) << "Loading symbols for module " << module_name << " from " <<
                 map_file;

  Module *module = new Module(module_name);
  if (!module->LoadMap(map_file)) {
    delete module;
    return false;
  }

  modules_->insert(make_pair(module_name, module));
  return true;
}

bool BasicSourceLineResolver::HasModule(const string &module_name) const {
  return modules_->find(module_name) != modules_->end();
}

StackFrameInfo* BasicSourceLineResolver::FillSourceLineInfo(
    StackFrame *frame) const {
  if (frame->module) {
    ModuleMap::const_iterator it = modules_->find(frame->module->code_file());
    if (it != modules_->end()) {
      return it->second->LookupAddress(frame);
    }
  }
  return NULL;
}

class AutoFileCloser {
 public:
  AutoFileCloser(FILE *file) : file_(file) {}
  ~AutoFileCloser() {
    if (file_)
      fclose(file_);
  }

 private:
  FILE *file_;
};

bool BasicSourceLineResolver::Module::LoadMap(const string &map_file) {
  FILE *f = fopen(map_file.c_str(), "r");
  if (!f) {
    string error_string;
    int error_code = ErrnoString(&error_string);
    BPLOG(ERROR) << "Could not open " << map_file <<
                    ", error " << error_code << ": " << error_string;
    return false;
  }

  AutoFileCloser closer(f);

  
  
  
  char buffer[8192];
  linked_ptr<Function> cur_func;

  int line_number = 0;
  while (fgets(buffer, sizeof(buffer), f)) {
    ++line_number;
    if (strncmp(buffer, "FILE ", 5) == 0) {
      if (!ParseFile(buffer)) {
        BPLOG(ERROR) << "ParseFile failed at " <<
                        map_file << ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "STACK ", 6) == 0) {
      if (!ParseStackInfo(buffer)) {
        BPLOG(ERROR) << "ParseStackInfo failed at " <<
                        map_file << ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "FUNC ", 5) == 0) {
      cur_func.reset(ParseFunction(buffer));
      if (!cur_func.get()) {
        BPLOG(ERROR) << "ParseFunction failed at " <<
                        map_file << ":" << line_number;
        return false;
      }
      
      
      
      functions_.StoreRange(cur_func->address, cur_func->size, cur_func);
    } else if (strncmp(buffer, "PUBLIC ", 7) == 0) {
      
      cur_func.reset();

      if (!ParsePublicSymbol(buffer)) {
        BPLOG(ERROR) << "ParsePublicSymbol failed at " <<
                        map_file << ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "MODULE ", 7) == 0) {
      
      
      
      
      
      
    } else {
      if (!cur_func.get()) {
        BPLOG(ERROR) << "Found source line data without a function at " <<
                        map_file << ":" << line_number;
        return false;
      }
      Line *line = ParseLine(buffer);
      if (!line) {
        BPLOG(ERROR) << "ParseLine failed at " <<
                        map_file << ":" << line_number;
        return false;
      }
      cur_func->lines.StoreRange(line->address, line->size,
                                 linked_ptr<Line>(line));
    }
  }

  return true;
}

StackFrameInfo* BasicSourceLineResolver::Module::LookupAddress(
    StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();

  linked_ptr<StackFrameInfo> retrieved_info;
  
  
  
  
  
  
  if (!stack_info_[STACK_INFO_FRAME_DATA].RetrieveRange(address,
                                                        &retrieved_info)) {
    stack_info_[STACK_INFO_FPO].RetrieveRange(address, &retrieved_info);
  }

  scoped_ptr<StackFrameInfo> frame_info;
  if (retrieved_info.get()) {
    frame_info.reset(new StackFrameInfo());
    frame_info->CopyFrom(*retrieved_info.get());
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  int parameter_size = 0;
  linked_ptr<Function> func;
  linked_ptr<PublicSymbol> public_symbol;
  MemAddr function_base;
  MemAddr function_size;
  MemAddr public_address;
  if (functions_.RetrieveNearestRange(address, &func,
                                      &function_base, &function_size) &&
      address >= function_base && address < function_base + function_size) {
    parameter_size = func->parameter_size;

    frame->function_name = func->name;
    frame->function_base = frame->module->base_address() + function_base;

    linked_ptr<Line> line;
    MemAddr line_base;
    if (func->lines.RetrieveRange(address, &line, &line_base, NULL)) {
      FileMap::const_iterator it = files_.find(line->source_file_id);
      if (it != files_.end()) {
        frame->source_file_name = files_.find(line->source_file_id)->second;
      }
      frame->source_line = line->line;
      frame->source_line_base = frame->module->base_address() + line_base;
    }
  } else if (public_symbols_.Retrieve(address,
                                      &public_symbol, &public_address) &&
             (!func.get() || public_address > function_base + function_size)) {
    parameter_size = public_symbol->parameter_size;

    frame->function_name = public_symbol->name;
    frame->function_base = frame->module->base_address() + public_address;
  } else {
    
    return frame_info.release();
  }

  if (!frame_info.get()) {
    
    
    
    
    frame_info.reset(new StackFrameInfo());
    frame_info->parameter_size = parameter_size;
    frame_info->valid |= StackFrameInfo::VALID_PARAMETER_SIZE;
  }

  return frame_info.release();
}


bool BasicSourceLineResolver::Module::Tokenize(char *line, int max_tokens,
                                               vector<char*> *tokens) {
  tokens->clear();
  tokens->reserve(max_tokens);

  int remaining = max_tokens;

  
  
  char *save_ptr;
  char *token = strtok_r(line, " \r\n", &save_ptr);
  while (token && --remaining > 0) {
    tokens->push_back(token);
    if (remaining > 1)
      token = strtok_r(NULL, " \r\n", &save_ptr);
  }

  
  if (!remaining > 0) {
    if ((token = strtok_r(NULL, "\r\n", &save_ptr))) {
      tokens->push_back(token);
    }
  }

  return tokens->size() == static_cast<unsigned int>(max_tokens);
}

bool BasicSourceLineResolver::Module::ParseFile(char *file_line) {
  
  file_line += 5;  

  vector<char*> tokens;
  if (!Tokenize(file_line, 2, &tokens)) {
    return false;
  }

  int index = atoi(tokens[0]);
  if (index < 0) {
    return false;
  }

  char *filename = tokens[1];
  if (!filename) {
    return false;
  }

  files_.insert(make_pair(index, string(filename)));
  return true;
}

BasicSourceLineResolver::Function*
BasicSourceLineResolver::Module::ParseFunction(char *function_line) {
  
  function_line += 5;  

  vector<char*> tokens;
  if (!Tokenize(function_line, 4, &tokens)) {
    return NULL;
  }

  u_int64_t address    = strtoull(tokens[0], NULL, 16);
  u_int64_t size       = strtoull(tokens[1], NULL, 16);
  int stack_param_size = strtoull(tokens[2], NULL, 16);
  char *name           = tokens[3];

  return new Function(name, address, size, stack_param_size);
}

BasicSourceLineResolver::Line* BasicSourceLineResolver::Module::ParseLine(
    char *line_line) {
  
  vector<char*> tokens;
  if (!Tokenize(line_line, 4, &tokens)) {
    return NULL;
  }

  u_int64_t address = strtoull(tokens[0], NULL, 16);
  u_int64_t size    = strtoull(tokens[1], NULL, 16);
  int line_number   = atoi(tokens[2]);
  int source_file   = atoi(tokens[3]);
  if (line_number <= 0) {
    return NULL;
  }

  return new Line(address, size, source_file, line_number);
}

bool BasicSourceLineResolver::Module::ParsePublicSymbol(char *public_line) {
  

  
  public_line += 7;

  vector<char*> tokens;
  if (!Tokenize(public_line, 3, &tokens)) {
    return false;
  }

  u_int64_t address    = strtoull(tokens[0], NULL, 16);
  int stack_param_size = strtoull(tokens[1], NULL, 16);
  char *name           = tokens[2];

  
  
  
  
  
  
  if (address == 0) {
    return true;
  }

  linked_ptr<PublicSymbol> symbol(new PublicSymbol(name, address,
                                                   stack_param_size));
  return public_symbols_.Store(address, symbol);
}

bool BasicSourceLineResolver::Module::ParseStackInfo(char *stack_info_line) {
  
  
  
  
  
  
  
  
  
  

  
  stack_info_line += 6;

  vector<char*> tokens;
  if (!Tokenize(stack_info_line, 12, &tokens))
    return false;

  
  const char *platform = tokens[0];
  if (strcmp(platform, "WIN") != 0)
    return false;

  int type = strtol(tokens[1], NULL, 16);
  if (type < 0 || type > STACK_INFO_LAST - 1)
    return false;

  u_int64_t rva                 = strtoull(tokens[2],  NULL, 16);
  u_int64_t code_size           = strtoull(tokens[3],  NULL, 16);
  u_int32_t prolog_size         =  strtoul(tokens[4],  NULL, 16);
  u_int32_t epilog_size         =  strtoul(tokens[5],  NULL, 16);
  u_int32_t parameter_size      =  strtoul(tokens[6],  NULL, 16);
  u_int32_t saved_register_size =  strtoul(tokens[7],  NULL, 16);
  u_int32_t local_size          =  strtoul(tokens[8],  NULL, 16);
  u_int32_t max_stack_size      =  strtoul(tokens[9],  NULL, 16);
  int has_program_string        =  strtoul(tokens[10], NULL, 16);

  const char *program_string = "";
  int allocates_base_pointer = 0;
  if (has_program_string) {
    program_string = tokens[11];
  } else {
    allocates_base_pointer = strtoul(tokens[11], NULL, 16);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  linked_ptr<StackFrameInfo> stack_frame_info(
      new StackFrameInfo(prolog_size,
                         epilog_size,
                         parameter_size,
                         saved_register_size,
                         local_size,
                         max_stack_size,
                         allocates_base_pointer,
                         program_string));
  stack_info_[type].StoreRange(rva, code_size, stack_frame_info);

  return true;
}

#ifdef BSLR_NO_HASH_MAP
bool BasicSourceLineResolver::CompareString::operator()(
    const string &s1, const string &s2) const {
  return strcmp(s1.c_str(), s2.c_str()) < 0;
}
#else  
size_t BasicSourceLineResolver::HashString::operator()(const string &s) const {
  return hash<const char*>()(s.c_str());
}
#endif  

}  
