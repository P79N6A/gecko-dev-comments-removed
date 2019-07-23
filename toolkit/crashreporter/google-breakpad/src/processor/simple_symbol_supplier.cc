


































#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>

#include "processor/simple_symbol_supplier.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/logging.h"
#include "processor/pathname_stripper.h"

namespace google_breakpad {

static bool file_exists(const string &file_name) {
  struct stat sb;
  return stat(file_name.c_str(), &sb) == 0;
}

SymbolSupplier::SymbolResult SimpleSymbolSupplier::GetSymbolFile(
    const CodeModule *module, const SystemInfo *system_info,
    string *symbol_file) {
  BPLOG_IF(ERROR, !symbol_file) << "SimpleSymbolSupplier::GetSymbolFile "
                                   "requires |symbol_file|";
  assert(symbol_file);
  symbol_file->clear();

  for (unsigned int path_index = 0; path_index < paths_.size(); ++path_index) {
    SymbolResult result;
    if ((result = GetSymbolFileAtPath(module, system_info, paths_[path_index],
                                      symbol_file)) != NOT_FOUND) {
      return result;
    }
  }
  return NOT_FOUND;
}

SymbolSupplier::SymbolResult SimpleSymbolSupplier::GetSymbolFileAtPath(
    const CodeModule *module, const SystemInfo *system_info,
    const string &root_path, string *symbol_file) {
  BPLOG_IF(ERROR, !symbol_file) << "SimpleSymbolSupplier::GetSymbolFileAtPath "
                                   "requires |symbol_file|";
  assert(symbol_file);
  symbol_file->clear();

  if (!module)
    return NOT_FOUND;

  
  string path = root_path;

  
  path.append("/");
  string debug_file_name = PathnameStripper::File(module->debug_file());
  if (debug_file_name.empty()) {
    BPLOG(ERROR) << "Can't construct symbol file path without debug_file "
                    "(code_file = " <<
                    PathnameStripper::File(module->code_file()) << ")";
    return NOT_FOUND;
  }
  path.append(debug_file_name);

  
  path.append("/");
  string identifier = module->debug_identifier();
  if (identifier.empty()) {
    BPLOG(ERROR) << "Can't construct symbol file path without debug_identifier "
                    "(code_file = " <<
                    PathnameStripper::File(module->code_file()) <<
                    ", debug_file = " << debug_file_name << ")";
    return NOT_FOUND;
  }
  path.append(identifier);

  
  
  
  path.append("/");
  string debug_file_extension;
  if (debug_file_name.size() > 4)
    debug_file_extension = debug_file_name.substr(debug_file_name.size() - 4);
  std::transform(debug_file_extension.begin(), debug_file_extension.end(),
                 debug_file_extension.begin(), tolower);
  if (debug_file_extension == ".pdb") {
    path.append(debug_file_name.substr(0, debug_file_name.size() - 4));
  } else {
    path.append(debug_file_name);
  }
  path.append(".sym");

  if (!file_exists(path)) {
    BPLOG(INFO) << "No symbol file at " << path;
    return NOT_FOUND;
  }

  *symbol_file = path;
  return FOUND;
}

}  
