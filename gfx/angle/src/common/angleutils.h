







#ifndef COMMON_ANGLEUTILS_H_
#define COMMON_ANGLEUTILS_H_



#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#define VENDOR_ID_AMD 0x1002
#define VENDOR_ID_INTEL 0x8086
#define VENDOR_ID_NVIDIA 0x10DE

#endif 
