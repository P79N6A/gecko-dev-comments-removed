



































#ifndef BREAKPAD_COMMON_LINUX_ELF_SYMBOLS_TO_MODULE_H_
#define BREAKPAD_COMMON_LINUX_ELF_SYMBOLS_TO_MODULE_H_

#include <stddef.h>
#include <stdint.h>

namespace google_breakpad {

class Module;

bool ELFSymbolsToModule(const uint8_t *symtab_section,
                        size_t symtab_size,
                        const uint8_t *string_section,
                        size_t string_size,
                        const bool big_endian,
                        size_t value_size,
                        Module *module);

}  


#endif  
