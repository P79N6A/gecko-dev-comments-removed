

































#ifndef COMMON_LINUX_DUMP_SYMBOLS_H__
#define COMMON_LINUX_DUMP_SYMBOLS_H__

#include <iostream>
#include <string>

#include "common/using_std_string.h"

namespace google_breakpad {







bool WriteSymbolFile(const string &obj_file,
                     const string &debug_dir,
                     bool cfi,
                     std::ostream &sym_stream);

}  

#endif  
