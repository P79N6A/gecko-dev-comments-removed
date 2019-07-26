

































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <iostream>
#include <string>
#include <vector>

#include "common/using_std_string.h"

namespace google_breakpad {

class Module;







bool WriteSymbolFile(const string &obj_file,
                     const std::vector<string>& debug_dirs,
                     bool cfi,
                     std::ostream &sym_stream);




bool ReadSymbolData(const string& obj_file,
                    const std::vector<string>& debug_dirs,
                    bool cfi,
                    Module** module);

}  

#endif  
