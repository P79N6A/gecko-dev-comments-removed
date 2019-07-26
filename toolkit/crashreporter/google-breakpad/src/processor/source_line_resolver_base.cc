



































#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <map>
#include <utility>

#include "google_breakpad/processor/source_line_resolver_base.h"
#include "processor/source_line_resolver_base_types.h"
#include "processor/module_factory.h"

using std::map;
using std::make_pair;

namespace google_breakpad {

SourceLineResolverBase::SourceLineResolverBase(
    ModuleFactory *module_factory)
  : modules_(new ModuleMap),
    memory_buffers_(new MemoryMap),
    module_factory_(module_factory) {
}

SourceLineResolverBase::~SourceLineResolverBase() {
  ModuleMap::iterator it;
  
  for (it = modules_->begin(); it != modules_->end(); ++it) {
    
    delete it->second;
  }
  
  delete modules_;

  MemoryMap::iterator iter = memory_buffers_->begin();
  for (; iter != memory_buffers_->end(); ++iter) {
    delete [] iter->second;
  }
  
  delete memory_buffers_;

  delete module_factory_;
}

bool SourceLineResolverBase::ReadSymbolFile(char **symbol_data,
                                            const string &map_file) {
  if (symbol_data == NULL) {
    BPLOG(ERROR) << "Could not Read file into Null memory pointer";
    return false;
  }

  struct stat buf;
  int error_code = stat(map_file.c_str(), &buf);
  if (error_code == -1) {
    string error_string;
    error_code = ErrnoString(&error_string);
    BPLOG(ERROR) << "Could not open " << map_file <<
        ", error " << error_code << ": " << error_string;
    return false;
  }

  off_t file_size = buf.st_size;

  
  
  *symbol_data = new char[file_size + 1];

  if (*symbol_data == NULL) {
    BPLOG(ERROR) << "Could not allocate memory for " << map_file;
    return false;
  }

  BPLOG(INFO) << "Opening " << map_file;

  FILE *f = fopen(map_file.c_str(), "rt");
  if (!f) {
    string error_string;
    error_code = ErrnoString(&error_string);
    BPLOG(ERROR) << "Could not open " << map_file <<
        ", error " << error_code << ": " << error_string;
    delete [] (*symbol_data);
    *symbol_data = NULL;
    return false;
  }

  AutoFileCloser closer(f);

  int items_read = 0;

  items_read = fread(*symbol_data, 1, file_size, f);

  if (items_read != file_size) {
    string error_string;
    error_code = ErrnoString(&error_string);
    BPLOG(ERROR) << "Could not slurp " << map_file <<
        ", error " << error_code << ": " << error_string;
    delete [] (*symbol_data);
    *symbol_data = NULL;
    return false;
  }

  (*symbol_data)[file_size] = '\0';
  return true;
}

bool SourceLineResolverBase::LoadModule(const CodeModule *module,
                                        const string &map_file) {
  if (module == NULL)
    return false;

  
  if (modules_->find(module->code_file()) != modules_->end()) {
    BPLOG(INFO) << "Symbols for module " << module->code_file()
                << " already loaded";
    return false;
  }

  BPLOG(INFO) << "Loading symbols for module " << module->code_file()
              << " from " << map_file;

  char *memory_buffer;
  if (!ReadSymbolFile(&memory_buffer, map_file))
    return false;

  BPLOG(INFO) << "Read symbol file " << map_file << " succeeded";

  bool load_result = LoadModuleUsingMemoryBuffer(module, memory_buffer);

  if (load_result && !ShouldDeleteMemoryBufferAfterLoadModule()) {
    
    memory_buffers_->insert(make_pair(module->code_file(), memory_buffer));
  } else {
    delete [] memory_buffer;
  }

  return load_result;
}

bool SourceLineResolverBase::LoadModuleUsingMapBuffer(
    const CodeModule *module, const string &map_buffer) {
  if (module == NULL)
    return false;

  
  if (modules_->find(module->code_file()) != modules_->end()) {
    BPLOG(INFO) << "Symbols for module " << module->code_file()
                << " already loaded";
    return false;
  }

  char *memory_buffer = new char[map_buffer.size() + 1];
  if (memory_buffer == NULL) {
    BPLOG(ERROR) << "Could not allocate memory for " << module->code_file();
    return false;
  }

  
  memcpy(memory_buffer, map_buffer.c_str(), map_buffer.size());
  memory_buffer[map_buffer.size()] = '\0';

  bool load_result = LoadModuleUsingMemoryBuffer(module, memory_buffer);

  if (load_result && !ShouldDeleteMemoryBufferAfterLoadModule()) {
    
    memory_buffers_->insert(make_pair(module->code_file(), memory_buffer));
  } else {
    delete [] memory_buffer;
  }

  return load_result;
}

bool SourceLineResolverBase::LoadModuleUsingMemoryBuffer(
    const CodeModule *module, char *memory_buffer) {
  if (!module)
    return false;

  
  if (modules_->find(module->code_file()) != modules_->end()) {
    BPLOG(INFO) << "Symbols for module " << module->code_file()
                << " already loaded";
    return false;
  }

  BPLOG(INFO) << "Loading symbols for module " << module->code_file()
             << " from memory buffer";

  Module *basic_module = module_factory_->CreateModule(module->code_file());

  
  if (!basic_module->LoadMapFromMemory(memory_buffer)) {
    delete basic_module;
    return false;
  }

  modules_->insert(make_pair(module->code_file(), basic_module));
  return true;
}

bool SourceLineResolverBase::ShouldDeleteMemoryBufferAfterLoadModule() {
  return true;
}

void SourceLineResolverBase::UnloadModule(const CodeModule *code_module) {
  if (!code_module)
    return;

  ModuleMap::iterator mod_iter = modules_->find(code_module->code_file());
  if (mod_iter != modules_->end()) {
    Module *symbol_module = mod_iter->second;
    delete symbol_module;
    modules_->erase(mod_iter);
  }

  if (ShouldDeleteMemoryBufferAfterLoadModule()) {
    
  } else {
    
    MemoryMap::iterator iter = memory_buffers_->find(code_module->code_file());
    if (iter != memory_buffers_->end()) {
      delete [] iter->second;
      memory_buffers_->erase(iter);
    }
  }
}

bool SourceLineResolverBase::HasModule(const CodeModule *module) {
  if (!module)
    return false;
  return modules_->find(module->code_file()) != modules_->end();
}

void SourceLineResolverBase::FillSourceLineInfo(StackFrame *frame) {
  if (frame->module) {
    ModuleMap::const_iterator it = modules_->find(frame->module->code_file());
    if (it != modules_->end()) {
      it->second->LookupAddress(frame);
    }
  }
}

WindowsFrameInfo *SourceLineResolverBase::FindWindowsFrameInfo(
    const StackFrame *frame) {
  if (frame->module) {
    ModuleMap::const_iterator it = modules_->find(frame->module->code_file());
    if (it != modules_->end()) {
      return it->second->FindWindowsFrameInfo(frame);
    }
  }
  return NULL;
}

CFIFrameInfo *SourceLineResolverBase::FindCFIFrameInfo(
    const StackFrame *frame) {
  if (frame->module) {
    ModuleMap::const_iterator it = modules_->find(frame->module->code_file());
    if (it != modules_->end()) {
      return it->second->FindCFIFrameInfo(frame);
    }
  }
  return NULL;
}

bool SourceLineResolverBase::CompareString::operator()(
    const string &s1, const string &s2) const {
  return strcmp(s1.c_str(), s2.c_str()) < 0;
}

bool SourceLineResolverBase::Module::ParseCFIRuleSet(
    const string &rule_set, CFIFrameInfo *frame_info) const {
  CFIFrameInfoParseHandler handler(frame_info);
  CFIRuleParser parser(&handler);
  return parser.Parse(rule_set);
}

}  
