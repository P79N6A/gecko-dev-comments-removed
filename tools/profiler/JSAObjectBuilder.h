




#ifndef JSAOBJECTBUILDER_H
#define JSAOBJECTBUILDER_H

class JSCustomObject;
class JSCustomArray;
class nsAString;

class JSAObjectBuilder
{
public:
  virtual ~JSAObjectBuilder() = 0;

  virtual void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomObject *aValue) = 0;
  virtual void DefineProperty(JSCustomObject *aObject, const char *name, JSCustomArray *aValue) = 0;
  virtual void DefineProperty(JSCustomObject *aObject, const char *name, int value) = 0;
  virtual void DefineProperty(JSCustomObject *aObject, const char *name, double value) = 0;
  virtual void DefineProperty(JSCustomObject *aObject, const char *name, const char *value) = 0;
  virtual void ArrayPush(JSCustomArray *aArray, int value) = 0;
  virtual void ArrayPush(JSCustomArray *aArray, const char *value) = 0;
  virtual void ArrayPush(JSCustomArray *aArray, JSCustomObject *aObject) = 0;
  virtual JSCustomArray  *CreateArray() = 0;
  virtual JSCustomObject *CreateObject() = 0;
};

#endif
