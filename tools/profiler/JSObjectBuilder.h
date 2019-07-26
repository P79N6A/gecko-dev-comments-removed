




#ifndef JSOBJECTBUILDER_H
#define JSOBJECTBUILDER_H

#include "JSAObjectBuilder.h"

class JSCustomObject;
class JSCustomObjectBuilder;
struct JSContext;
class nsAString;



class JSObjectBuilder : public JSAObjectBuilder
{
public:
  
  explicit JSObjectBuilder(JSContext *aCx);
  ~JSObjectBuilder() {}

  void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomObject *aValue);
  void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomArray *aValue);
  void DefineProperty(JSCustomObject *aObject, const char *name, int value);
  void DefineProperty(JSCustomObject *aObject, const char *name, double value);
  void DefineProperty(JSCustomObject *aObject, const char *name, nsAString &value);
  void DefineProperty(JSCustomObject *aObject, const char *name, const char *value, size_t valueLength);
  void DefineProperty(JSCustomObject *aObject, const char *name, const char *value);
  void ArrayPush(JSCustomArray *aArray, int value);
  void ArrayPush(JSCustomArray *aArray, const char *value);
  void ArrayPush(JSCustomArray *aArray, JSCustomArray *aObject);
  void ArrayPush(JSCustomArray *aArray, JSCustomObject *aObject);
  JSCustomArray *CreateArray();
  JSCustomObject *CreateObject();

  JSObject* GetJSObject(JSCustomObject* aObject) { return (JSObject*)aObject; }

private:
  JSObjectBuilder(const JSObjectBuilder&);
  JSObjectBuilder& operator=(const JSObjectBuilder&);

  void* operator new(size_t);
  void* operator new[](size_t);
  void operator delete(void*) {
    
    
    
    
    abort();
  }
  void operator delete[](void*);

  JSContext *mCx;
  JSObject *mObj;
  int mOk;
};

#endif

