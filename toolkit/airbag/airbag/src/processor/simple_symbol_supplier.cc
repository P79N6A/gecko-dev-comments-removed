


































#include <sys/types.h>
#include <sys/stat.h>

#include <cassert>

#include "processor/simple_symbol_supplier.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/pathname_stripper.h"

namespace google_breakpad {

static bool file_exists(const string &file_name) {
  struct stat sb;
  return stat(file_name.c_str(), &sb) == 0;
}

SymbolSupplier::SymbolResult SimpleSymbolSupplier::GetSymbolFile(
    const CodeModule *module, const SystemInfo *system_info,
    string *symbol_file) {
  assert(symbol_file);
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
  assert(symbol_file);
  if (!module)
    return NOT_FOUND;

  
  string path = root_path;

  
  path.append("/");
  string debug_file_name = PathnameStripper::File(module->debug_file());
  if (debug_file_name.empty())
    return NOT_FOUND;
  path.append(debug_file_name);

  
  path.append("/");
  string identifier = module->debug_identifier();
  if (identifier.empty())
    return NOT_FOUND;
  path.append(identifier);

  
  
  
  path.append("/");
  string debug_file_extension =
      debug_file_name.substr(debug_file_name.size() - 4);
  transform(debug_file_extension.begin(), debug_file_extension.end(),
            debug_file_extension.begin(), tolower);
  if (debug_file_extension == ".pdb") {
    path.append(debug_file_name.substr(0, debug_file_name.size() - 4));
  } else {
    path.append(debug_file_name);
  }
  path.append(".sym");

  if (!file_exists(path))
    return NOT_FOUND;

  *symbol_file = path;
  return FOUND;
}

}  
