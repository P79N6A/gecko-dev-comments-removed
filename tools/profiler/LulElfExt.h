



































#ifndef LulElfExt_h
#define LulElfExt_h




#include "LulMainInt.h"

using lul::SecMap;

namespace lul {



bool ReadSymbolData(const std::string& obj_file,
                    const std::vector<std::string>& debug_dirs,
                    SecMap* smap,
                    void* rx_avma, size_t rx_size,
                    void (*log)(const char*));



bool ReadSymbolDataInternal(const uint8_t* obj_file,
                            const std::string& obj_filename,
                            const std::vector<std::string>& debug_dirs,
                            SecMap* smap,
                            void* rx_avma, size_t rx_size,
                            void (*log)(const char*));

}  

#endif 
