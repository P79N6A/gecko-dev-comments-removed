


























#ifndef V8_GLOBALS_H_
#define V8_GLOBALS_H_

namespace v8 {
namespace internal {





#define V8_2PART_UINT64_C(a, b) (((static_cast<uint64_t>(a) << 32) + 0x##b##u))




const int kCharSize     = sizeof(char);      








#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)








#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

} }  

#endif  
