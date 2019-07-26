




#ifndef JSCUSTOMOBJECTBUILDER_H
#define JSCUSTOMOBJECTBUILDER_H

#include <ostream>
#include <stdlib.h>
#include "JSAObjectBuilder.h"

class JSCustomObject;
class JSCustomArray;
class JSCustomObjectBuilder;

class JSCustomObjectBuilder : public JSAObjectBuilder
{
public:

  
  JSCustomObjectBuilder();

  void Serialize(JSCustomObject* aObject, std::ostream& stream);

  void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomObject *aValue);
  void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomArray *aValue);
  void DefineProperty(JSCustomObject *aObject, const char *name, int value);
  void DefineProperty(JSCustomObject *aObject, const char *name, double value);
  void DefineProperty(JSCustomObject *aObject, const char *name, const char *value, size_t valueLength);
  void DefineProperty(JSCustomObject *aObject, const char *name, const char *value);
  void ArrayPush(JSCustomArray *aArray, int value);
  void ArrayPush(JSCustomArray *aArray, const char *value);
  void ArrayPush(JSCustomArray *aArray, JSCustomObject *aObject);
  JSCustomArray  *CreateArray();
  JSCustomObject *CreateObject();

  
  void DeleteObject(JSCustomObject* aObject);

private:
  
  JSCustomObjectBuilder(const JSCustomObjectBuilder&);
  JSCustomObjectBuilder& operator=(const JSCustomObjectBuilder&);

  void* operator new(size_t);
  void* operator new[](size_t);
  void operator delete(void*) {
    
    
    
    
    abort();
  }
  void operator delete[](void*);
};

#endif
