































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <string>
#include <cstdio>

namespace google_breakpad {

class DumpSymbols {
 public:
  bool WriteSymbolFile(const std::string &obj_file,
                       FILE *sym_file);
};

}  

#endif  
