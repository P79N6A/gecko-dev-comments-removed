


































#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <map>
#include <utility>
#include <vector>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "processor/basic_source_line_resolver_types.h"
#include "processor/module_factory.h"

#include "processor/tokenize.h"

using std::map;
using std::vector;
using std::make_pair;

namespace google_breakpad {

#ifdef _WIN32
#define strtok_r strtok_s
#define strtoull _strtoui64
#endif

static const char *kWhitespace = " \r\n";

BasicSourceLineResolver::BasicSourceLineResolver() :
    SourceLineResolverBase(new BasicModuleFactory) { }

bool BasicSourceLineResolver::Module::LoadMapFromMemory(char *memory_buffer) {
  linked_ptr<Function> cur_func;
  int line_number = 0;
  char *save_ptr;
  size_t map_buffer_length = strlen(memory_buffer);

  
  
  
  if (map_buffer_length == 0) {
    return true;
  }

  if (memory_buffer[map_buffer_length - 1] == '\n') {
    memory_buffer[map_buffer_length - 1] = '\0';
  }

  char *buffer;
  buffer = strtok_r(memory_buffer, "\r\n", &save_ptr);

  while (buffer != NULL) {
    ++line_number;

    if (strncmp(buffer, "FILE ", 5) == 0) {
      if (!ParseFile(buffer)) {
        BPLOG(ERROR) << "ParseFile on buffer failed at " <<
            ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "STACK ", 6) == 0) {
      if (!ParseStackInfo(buffer)) {
        BPLOG(ERROR) << "ParseStackInfo failed at " <<
            ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "FUNC ", 5) == 0) {
      cur_func.reset(ParseFunction(buffer));
      if (!cur_func.get()) {
        BPLOG(ERROR) << "ParseFunction failed at " <<
            ":" << line_number;
        return false;
      }
      
      
      
      functions_.StoreRange(cur_func->address, cur_func->size, cur_func);
    } else if (strncmp(buffer, "PUBLIC ", 7) == 0) {
      
      cur_func.reset();

      if (!ParsePublicSymbol(buffer)) {
        BPLOG(ERROR) << "ParsePublicSymbol failed at " <<
            ":" << line_number;
        return false;
      }
    } else if (strncmp(buffer, "MODULE ", 7) == 0) {
      
      
      
      
      
      
    } else if (strncmp(buffer, "INFO ", 5) == 0) {
      
      
      
    } else {
      if (!cur_func.get()) {
        BPLOG(ERROR) << "Found source line data without a function at " <<
            ":" << line_number;
        return false;
      }
      Line *line = ParseLine(buffer);
      if (!line) {
        BPLOG(ERROR) << "ParseLine failed at " << line_number << " for " <<
            buffer;
        return false;
      }
      cur_func->lines.StoreRange(line->address, line->size,
                                 linked_ptr<Line>(line));
    }
    buffer = strtok_r(NULL, "\r\n", &save_ptr);
  }
  return true;
}

void BasicSourceLineResolver::Module::LookupAddress(StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();

  
  
  
  
  
  
  linked_ptr<Function> func;
  linked_ptr<PublicSymbol> public_symbol;
  MemAddr function_base;
  MemAddr function_size;
  MemAddr public_address;
  if (functions_.RetrieveNearestRange(address, &func,
                                      &function_base, &function_size) &&
      address >= function_base && address - function_base < function_size) {
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
             (!func.get() || public_address > function_base)) {
    frame->function_name = public_symbol->name;
    frame->function_base = frame->module->base_address() + public_address;
  }
}

WindowsFrameInfo *BasicSourceLineResolver::Module::FindWindowsFrameInfo(
    const StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();
  scoped_ptr<WindowsFrameInfo> result(new WindowsFrameInfo());

  
  
  
  
  
  
  linked_ptr<WindowsFrameInfo> frame_info;
  if ((windows_frame_info_[WindowsFrameInfo::STACK_INFO_FRAME_DATA]
       .RetrieveRange(address, &frame_info))
      || (windows_frame_info_[WindowsFrameInfo::STACK_INFO_FPO]
          .RetrieveRange(address, &frame_info))) {
    result->CopyFrom(*frame_info.get());
    return result.release();
  }

  
  
  
  
  
  
  
  linked_ptr<Function> function;
  MemAddr function_base, function_size;
  if (functions_.RetrieveNearestRange(address, &function,
                                      &function_base, &function_size) &&
      address >= function_base && address - function_base < function_size) {
    result->parameter_size = function->parameter_size;
    result->valid |= WindowsFrameInfo::VALID_PARAMETER_SIZE;
    return result.release();
  }

  
  
  linked_ptr<PublicSymbol> public_symbol;
  MemAddr public_address;
  if (public_symbols_.Retrieve(address, &public_symbol, &public_address) &&
      (!function.get() || public_address > function_base)) {
    result->parameter_size = public_symbol->parameter_size;
  }

  return NULL;
}

CFIFrameInfo *BasicSourceLineResolver::Module::FindCFIFrameInfo(
    const StackFrame *frame) const {
  MemAddr address = frame->instruction - frame->module->base_address();
  MemAddr initial_base, initial_size;
  string initial_rules;

  
  
  
  
  if (!cfi_initial_rules_.RetrieveRange(address, &initial_rules,
                                        &initial_base, &initial_size)) {
    return NULL;
  }

  
  
  scoped_ptr<CFIFrameInfo> rules(new CFIFrameInfo());
  if (!ParseCFIRuleSet(initial_rules, rules.get()))
    return NULL;

  
  map<MemAddr, string>::const_iterator delta =
    cfi_delta_rules_.lower_bound(initial_base);

  
  while (delta != cfi_delta_rules_.end() && delta->first <= address) {
    ParseCFIRuleSet(delta->second, rules.get());
    delta++;
  }

  return rules.release();
}

bool BasicSourceLineResolver::Module::ParseFile(char *file_line) {
  
  file_line += 5;  

  vector<char*> tokens;
  if (!Tokenize(file_line, kWhitespace, 2, &tokens)) {
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
  if (!Tokenize(function_line, kWhitespace, 4, &tokens)) {
    return NULL;
  }

  uint64_t address     = strtoull(tokens[0], NULL, 16);
  uint64_t size        = strtoull(tokens[1], NULL, 16);
  int stack_param_size = strtoull(tokens[2], NULL, 16);
  char *name           = tokens[3];

  return new Function(name, address, size, stack_param_size);
}

BasicSourceLineResolver::Line* BasicSourceLineResolver::Module::ParseLine(
    char *line_line) {
  
  vector<char*> tokens;
  if (!Tokenize(line_line, kWhitespace, 4, &tokens)) {
    return NULL;
  }

  uint64_t address  = strtoull(tokens[0], NULL, 16);
  uint64_t size     = strtoull(tokens[1], NULL, 16);
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
  if (!Tokenize(public_line, kWhitespace, 3, &tokens)) {
    return false;
  }

  uint64_t address     = strtoull(tokens[0], NULL, 16);
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

  
  
  while (*stack_info_line == ' ')
    stack_info_line++;
  const char *platform = stack_info_line;
  while (!strchr(kWhitespace, *stack_info_line))
    stack_info_line++;
  *stack_info_line++ = '\0';

  
  if (strcmp(platform, "WIN") == 0) {
    int type = 0;
    uint64_t rva, code_size;
    linked_ptr<WindowsFrameInfo>
      stack_frame_info(WindowsFrameInfo::ParseFromString(stack_info_line,
                                                         type,
                                                         rva,
                                                         code_size));
    if (stack_frame_info == NULL)
      return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    windows_frame_info_[type].StoreRange(rva, code_size, stack_frame_info);
    return true;
  } else if (strcmp(platform, "CFI") == 0) {
    
    return ParseCFIFrameInfo(stack_info_line);
  } else {
    
    return false;
  }
}

bool BasicSourceLineResolver::Module::ParseCFIFrameInfo(
    char *stack_info_line) {
  char *cursor;

  
  char *init_or_address = strtok_r(stack_info_line, " \r\n", &cursor);
  if (!init_or_address)
    return false;

  if (strcmp(init_or_address, "INIT") == 0) {
    
    char *address_field = strtok_r(NULL, " \r\n", &cursor);
    if (!address_field) return false;

    char *size_field = strtok_r(NULL, " \r\n", &cursor);
    if (!size_field) return false;

    char *initial_rules = strtok_r(NULL, "\r\n", &cursor);
    if (!initial_rules) return false;

    MemAddr address = strtoul(address_field, NULL, 16);
    MemAddr size    = strtoul(size_field,    NULL, 16);
    cfi_initial_rules_.StoreRange(address, size, initial_rules);
    return true;
  }

  
  char *address_field = init_or_address;
  char *delta_rules = strtok_r(NULL, "\r\n", &cursor);
  if (!delta_rules) return false;
  MemAddr address = strtoul(address_field, NULL, 16);
  cfi_delta_rules_[address] = delta_rules;
  return true;
}

}  
