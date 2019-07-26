




class JSObject;
class JSObjectBuilder;
class nsAString;



class JSObjectBuilder
{
public:

  
  JSObjectBuilder(JSContext *aCx);

  void DefineProperty(JSObject *aObject, const char *name, JSObject *aValue);
  void DefineProperty(JSObject *aObject, const char *name, int value);
  void DefineProperty(JSObject *aObject, const char *name, double value);
  void DefineProperty(JSObject *aObject, const char *name, nsAString &value);
  void DefineProperty(JSObject *aObject, const char *name, const char *value, size_t valueLength);
  void DefineProperty(JSObject *aObject, const char *name, const char *value);
  void ArrayPush(JSObject *aArray, int value);
  void ArrayPush(JSObject *aArray, const char *value);
  void ArrayPush(JSObject *aArray, JSObject *aObject);
  JSObject *CreateArray();
  JSObject *CreateObject();

private:
  JSObjectBuilder(JSObjectBuilder&);

  JSContext *mCx;
  JSObject *mObj;
  int mOk;
};


