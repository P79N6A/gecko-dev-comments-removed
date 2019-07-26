




#ifndef JSOBJECTBUILDER_H
#define JSOBJECTBUILDER_H

#include "js/TypeDecls.h"
#include "js/RootingAPI.h"

class JSCustomArray;
class JSCustomObject;
class JSCustomObjectBuilder;
class nsAString;



class JSObjectBuilder
{
public:
  typedef JS::Handle<JSObject*> ObjectHandle;
  typedef JS::Handle<JSObject*> ArrayHandle;
  typedef JS::Rooted<JSObject*> RootedObject;
  typedef JS::Rooted<JSObject*> RootedArray;
  typedef JSObject* Object;
  typedef JSObject* Array;

  
  explicit JSObjectBuilder(JSContext *aCx);
  ~JSObjectBuilder() {}

  void DefineProperty(JS::HandleObject aObject, const char *name, JS::HandleObject aValue);
  void DefineProperty(JS::HandleObject aObject, const char *name, int value);
  void DefineProperty(JS::HandleObject aObject, const char *name, double value);
  void DefineProperty(JS::HandleObject aObject, const char *name, nsAString &value);
  void DefineProperty(JS::HandleObject aObject, const char *name, const char *value, size_t valueLength);
  void DefineProperty(JS::HandleObject aObject, const char *name, const char *value);
  void ArrayPush(JS::HandleObject aArray, int value);
  void ArrayPush(JS::HandleObject aArray, const char *value);
  void ArrayPush(JS::HandleObject aArray, JS::HandleObject aObject);
  JSObject *CreateArray();
  JSObject *CreateObject();

  JSContext *context() const { return mCx; }

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
  int mOk;
};

#endif

