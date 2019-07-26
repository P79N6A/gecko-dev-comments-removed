














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONSTRUCTOR_MAGIC_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CONSTRUCTOR_MAGIC_H_

#ifndef DISALLOW_ASSIGN
#define DISALLOW_ASSIGN(TypeName) \
  void operator=(const TypeName&)
#endif

#ifndef DISALLOW_COPY_AND_ASSIGN


#define DISALLOW_COPY_AND_ASSIGN(TypeName)    \
  TypeName(const TypeName&);                    \
  DISALLOW_ASSIGN(TypeName)
#endif

#ifndef DISALLOW_EVIL_CONSTRUCTORS

#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
  DISALLOW_COPY_AND_ASSIGN(TypeName)
#endif

#ifndef DISALLOW_IMPLICIT_CONSTRUCTORS






#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_EVIL_CONSTRUCTORS(TypeName)
#endif

#endif  
