




#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

class nsIArray;

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ 0xcaf520a5, 0x8078, 0x4cba, \
  { 0x8a, 0xb9, 0xb6, 0x8a, 0x12, 0x43, 0x4f, 0x05 } }






class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  
  
  
  virtual JSObject *GetScriptObject() = 0;

  
  virtual const PRUnichar *GetHandlerText() = 0;

  
  
  
  virtual void GetLocation(const char **aFileName, uint32_t *aLineNo) = 0;

  
  
  virtual nsIArray *GetArgv() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
