







#ifndef COMMON_ANGLEUTILS_H_
#define COMMON_ANGLEUTILS_H_



#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#endif 
