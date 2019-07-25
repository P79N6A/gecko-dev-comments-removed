

































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <stdio.h>

#include <string>

namespace google_breakpad {






bool WriteSymbolFile(const std::string &obj_file,
                     const std::string &debug_dir, FILE *sym_file);

}  

#endif  
