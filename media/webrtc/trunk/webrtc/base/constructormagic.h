









#ifndef WEBRTC_BASE_CONSTRUCTORMAGIC_H_
#define WEBRTC_BASE_CONSTRUCTORMAGIC_H_




#undef DISALLOW_ASSIGN
#define DISALLOW_ASSIGN(TypeName) \
  void operator=(const TypeName&)



#undef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)    \
  TypeName(const TypeName&);                    \
  DISALLOW_ASSIGN(TypeName)


#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
  DISALLOW_COPY_AND_ASSIGN(TypeName)







#undef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_EVIL_CONSTRUCTORS(TypeName)


#endif  
