



#ifndef BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_
#define BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_

#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"













namespace base {



BASE_EXPORT std::string IntToString(int value);
BASE_EXPORT string16 IntToString16(int value);

BASE_EXPORT std::string UintToString(unsigned value);
BASE_EXPORT string16 UintToString16(unsigned value);

BASE_EXPORT std::string Int64ToString(int64 value);
BASE_EXPORT string16 Int64ToString16(int64 value);

BASE_EXPORT std::string Uint64ToString(uint64 value);
BASE_EXPORT string16 Uint64ToString16(uint64 value);



BASE_EXPORT std::string DoubleToString(double value);

















BASE_EXPORT bool StringToInt(const StringPiece& input, int* output);
BASE_EXPORT bool StringToInt(const StringPiece16& input, int* output);

BASE_EXPORT bool StringToUint(const StringPiece& input, unsigned* output);
BASE_EXPORT bool StringToUint(const StringPiece16& input, unsigned* output);

BASE_EXPORT bool StringToInt64(const StringPiece& input, int64* output);
BASE_EXPORT bool StringToInt64(const StringPiece16& input, int64* output);

BASE_EXPORT bool StringToUint64(const StringPiece& input, uint64* output);
BASE_EXPORT bool StringToUint64(const StringPiece16& input, uint64* output);

BASE_EXPORT bool StringToSizeT(const StringPiece& input, size_t* output);
BASE_EXPORT bool StringToSizeT(const StringPiece16& input, size_t* output);







BASE_EXPORT bool StringToDouble(const std::string& input, double* output);









BASE_EXPORT std::string HexEncode(const void* bytes, size_t size);




BASE_EXPORT bool HexStringToInt(const StringPiece& input, int* output);




BASE_EXPORT bool HexStringToInt64(const StringPiece& input, int64* output);





BASE_EXPORT bool HexStringToUInt64(const StringPiece& input, uint64* output);





BASE_EXPORT bool HexStringToBytes(const std::string& input,
                                  std::vector<uint8>* output);

}  

#endif  
