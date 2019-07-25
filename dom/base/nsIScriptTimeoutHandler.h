





































#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

class nsIArray;

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ 0xd60ec934, 0x0c75, 0x4777, \
  { 0xba, 0x41, 0xb8, 0x2f, 0x37, 0xc9, 0x13, 0x56 } }






class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  
  virtual PRUint32 GetScriptTypeID() = 0;

  
  
  
  virtual JSObject *GetScriptObject() = 0;

  
  virtual const PRUnichar *GetHandlerText() = 0;

  
  
  
  virtual void GetLocation(const char **aFileName, PRUint32 *aLineNo) = 0;

  
  
  virtual nsIArray *GetArgv() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
