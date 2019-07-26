

































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <iostream>
#include <string>
#include <vector>

#include "common/symbol_data.h"
#include "common/using_std_string.h"

namespace google_breakpad {

class Module;







bool WriteSymbolFile(const string &obj_file,
                     const std::vector<string>& debug_dirs,
                     SymbolData symbol_data,
                     std::ostream &sym_stream);




bool ReadSymbolData(const string& obj_file,
                    const std::vector<string>& debug_dirs,
                    SymbolData symbol_data,
                    Module** module);

}  

#endif  
