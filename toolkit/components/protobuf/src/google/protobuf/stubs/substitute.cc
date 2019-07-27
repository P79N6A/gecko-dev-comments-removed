































#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {
namespace strings {

using internal::SubstituteArg;



static int CountSubstituteArgs(const SubstituteArg* const* args_array) {
  int count = 0;
  while (args_array[count] != NULL && args_array[count]->size() != -1) {
    ++count;
  }
  return count;
}

string Substitute(
    const char* format,
    const SubstituteArg& arg0, const SubstituteArg& arg1,
    const SubstituteArg& arg2, const SubstituteArg& arg3,
    const SubstituteArg& arg4, const SubstituteArg& arg5,
    const SubstituteArg& arg6, const SubstituteArg& arg7,
    const SubstituteArg& arg8, const SubstituteArg& arg9) {
  string result;
  SubstituteAndAppend(&result, format, arg0, arg1, arg2, arg3, arg4,
                                       arg5, arg6, arg7, arg8, arg9);
  return result;
}

void SubstituteAndAppend(
    string* output, const char* format,
    const SubstituteArg& arg0, const SubstituteArg& arg1,
    const SubstituteArg& arg2, const SubstituteArg& arg3,
    const SubstituteArg& arg4, const SubstituteArg& arg5,
    const SubstituteArg& arg6, const SubstituteArg& arg7,
    const SubstituteArg& arg8, const SubstituteArg& arg9) {
  const SubstituteArg* const args_array[] = {
    &arg0, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, NULL
  };

  
  int size = 0;
  for (int i = 0; format[i] != '\0'; i++) {
    if (format[i] == '$') {
      if (ascii_isdigit(format[i+1])) {
        int index = format[i+1] - '0';
        if (args_array[index]->size() == -1) {
          GOOGLE_LOG(DFATAL)
            << "strings::Substitute format string invalid: asked for \"$"
            << index << "\", but only " << CountSubstituteArgs(args_array)
            << " args were given.  Full format string was: \""
            << CEscape(format) << "\".";
          return;
        }
        size += args_array[index]->size();
        ++i;  
      } else if (format[i+1] == '$') {
        ++size;
        ++i;  
      } else {
        GOOGLE_LOG(DFATAL)
          << "Invalid strings::Substitute() format string: \""
          << CEscape(format) << "\".";
        return;
      }
    } else {
      ++size;
    }
  }

  if (size == 0) return;

  
  int original_size = output->size();
  STLStringResizeUninitialized(output, original_size + size);
  char* target = string_as_array(output) + original_size;
  for (int i = 0; format[i] != '\0'; i++) {
    if (format[i] == '$') {
      if (ascii_isdigit(format[i+1])) {
        const SubstituteArg* src = args_array[format[i+1] - '0'];
        memcpy(target, src->data(), src->size());
        target += src->size();
        ++i;  
      } else if (format[i+1] == '$') {
        *target++ = '$';
        ++i;  
      }
    } else {
      *target++ = format[i];
    }
  }

  GOOGLE_DCHECK_EQ(target - output->data(), output->size());
}

}  
}  
}  
