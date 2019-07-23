

































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <string>
#include <cstdio>

namespace google_breakpad {




bool WriteSymbolFile(const std::string &obj_file, FILE *sym_file);

}  

#endif  
