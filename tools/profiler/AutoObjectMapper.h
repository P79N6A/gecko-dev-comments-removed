





#ifndef AutoObjectMapper_h
#define AutoObjectMapper_h

#include <string>

#include "mozilla/Attributes.h"
#include "PlatformMacros.h"





class MOZ_STACK_CLASS AutoObjectMapperPOSIX {
public:
  
  
  
  
  
  explicit AutoObjectMapperPOSIX(void(*aLog)(const char*));

  
  ~AutoObjectMapperPOSIX();

  
  
  
  
  
  
  bool Map(void** start, size_t* length, std::string fileName);

protected:
  
  
  void*  mImage;
  size_t mSize;

  
  void (*mLog)(const char*);

private:
  
  
  
  bool mIsMapped;

  
  AutoObjectMapperPOSIX(const AutoObjectMapperPOSIX&);
  AutoObjectMapperPOSIX& operator=(const AutoObjectMapperPOSIX&);
  
  void* operator new(size_t);
  void* operator new[](size_t);
  void  operator delete(void*);
  void  operator delete[](void*);
};


#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)






















class MOZ_STACK_CLASS AutoObjectMapperFaultyLib : public AutoObjectMapperPOSIX {
public:
  AutoObjectMapperFaultyLib(void(*aLog)(const char*));

  ~AutoObjectMapperFaultyLib();

  bool Map(void** start, size_t* length, std::string fileName);

private:
  
  
  
  
  void* mHdl;

  
  AutoObjectMapperFaultyLib(const AutoObjectMapperFaultyLib&);
  AutoObjectMapperFaultyLib& operator=(const AutoObjectMapperFaultyLib&);
  
  void* operator new(size_t);
  void* operator new[](size_t);
  void  operator delete(void*);
  void  operator delete[](void*);
};

#endif 

#endif 
