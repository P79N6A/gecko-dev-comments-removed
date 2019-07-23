




























#include "common/convert_UTF.h"
#include "processor/scoped_ptr.h"
#include "common/string_conversion.h"

namespace google_breakpad {

using std::string;
using std::vector;

void UTF8ToUTF16(const char *in, vector<u_int16_t> *out) {
  size_t source_length = strlen(in);
  const UTF8 *source_ptr = reinterpret_cast<const UTF8 *>(in);
  const UTF8 *source_end_ptr = source_ptr + source_length;
  
  out->empty();
  out->insert(out->begin(), source_length, 0);
  u_int16_t *target_ptr = &(*out)[0];
  u_int16_t *target_end_ptr = target_ptr + out->capacity() * sizeof(u_int16_t);
  ConversionResult result = ConvertUTF8toUTF16(&source_ptr, source_end_ptr,
                                               &target_ptr, target_end_ptr,
                                               strictConversion);

  
  out->resize(result == conversionOK ? target_ptr - &(*out)[0] + 1: 0);
}

int UTF8ToUTF16Char(const char *in, int in_length, u_int16_t out[2]) {
  const UTF8 *source_ptr = reinterpret_cast<const UTF8 *>(in);
  const UTF8 *source_end_ptr = source_ptr + sizeof(char);
  u_int16_t *target_ptr = out;
  u_int16_t *target_end_ptr = target_ptr + 2 * sizeof(u_int16_t);
  out[0] = out[1] = 0;

  
  while (1) {
    ConversionResult result = ConvertUTF8toUTF16(&source_ptr, source_end_ptr,
                                                 &target_ptr, target_end_ptr,
                                                 strictConversion);

    if (result == conversionOK)
      return source_ptr - reinterpret_cast<const UTF8 *>(in);

    
    source_ptr = reinterpret_cast<const UTF8 *>(in);
    ++source_end_ptr;

    if (source_end_ptr > reinterpret_cast<const UTF8 *>(in) + in_length)
      break;
  }

  return 0;
}

void UTF32ToUTF16(const wchar_t *in, vector<u_int16_t> *out) {
  size_t source_length = wcslen(in);
  const UTF32 *source_ptr = reinterpret_cast<const UTF32 *>(in);
  const UTF32 *source_end_ptr = source_ptr + source_length;
  
  out->empty();
  out->insert(out->begin(), source_length, 0);
  u_int16_t *target_ptr = &(*out)[0];
  u_int16_t *target_end_ptr = target_ptr + out->capacity() * sizeof(u_int16_t);
  ConversionResult result = ConvertUTF32toUTF16(&source_ptr, source_end_ptr,
                                                &target_ptr, target_end_ptr,
                                                strictConversion);

  
  out->resize(result == conversionOK ? target_ptr - &(*out)[0] + 1: 0);
}

void UTF32ToUTF16Char(wchar_t in, u_int16_t out[2]) {
  const UTF32 *source_ptr = reinterpret_cast<const UTF32 *>(&in);
  const UTF32 *source_end_ptr = source_ptr + 1;
  u_int16_t *target_ptr = out;
  u_int16_t *target_end_ptr = target_ptr + 2 * sizeof(u_int16_t);
  out[0] = out[1] = 0;
  ConversionResult result = ConvertUTF32toUTF16(&source_ptr, source_end_ptr,
                                                &target_ptr, target_end_ptr,
                                                strictConversion);

  if (result != conversionOK) {
    out[0] = out[1] = 0;
  }
}

static inline u_int16_t Swap(u_int16_t value) {
  return (value >> 8) | (value << 8);
}

string UTF16ToUTF8(const vector<u_int16_t> &in, bool swap) {
  const UTF16 *source_ptr = &in[0];
  scoped_ptr<u_int16_t> source_buffer;

  
  if (swap) {
    int idx = 0;
    source_buffer.reset(new u_int16_t[in.size()]);
    UTF16 *source_buffer_ptr = source_buffer.get();
    for (vector<u_int16_t>::const_iterator it = in.begin();
         it != in.end(); ++it, ++idx)
      source_buffer_ptr[idx] = Swap(*it);

    source_ptr = source_buffer.get();
  }

  
  const UTF16 *source_end_ptr = source_ptr + in.size();
  int target_capacity = in.size() * 4;
  scoped_array<UTF8> target_buffer(new UTF8[target_capacity]);
  UTF8 *target_ptr = target_buffer.get();
  UTF8 *target_end_ptr = target_ptr + target_capacity;
  ConversionResult result = ConvertUTF16toUTF8(&source_ptr, source_end_ptr, 
                                               &target_ptr, target_end_ptr,
                                               strictConversion);

  if (result == conversionOK) {
    const char *targetPtr = reinterpret_cast<const char *>(target_buffer.get());
    string result(targetPtr);
    return result;
  }

  return "";
}

}  
