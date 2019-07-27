






































#ifndef GOOGLE_PROTOBUF_STUBS_STRINGPRINTF_H
#define GOOGLE_PROTOBUF_STUBS_STRINGPRINTF_H

#include <stdarg.h>
#include <string>
#include <vector>

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {


LIBPROTOBUF_EXPORT extern string StringPrintf(const char* format, ...);


LIBPROTOBUF_EXPORT extern const string& SStringPrintf(string* dst, const char* format, ...);


LIBPROTOBUF_EXPORT extern void StringAppendF(string* dst, const char* format, ...);



LIBPROTOBUF_EXPORT extern void StringAppendV(string* dst, const char* format, va_list ap);


LIBPROTOBUF_EXPORT extern const int kStringPrintfVectorMaxArgs;




LIBPROTOBUF_EXPORT extern string StringPrintfVector(const char* format, const vector<string>& v);

}  
}  

#endif  
