





































#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

class nsIArray;

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ /* {17a9ce1a-d73b-45d1-8145-a0ae57bcc76e} */ \
  0x17a9ce1a, 0xd73b, 0x45d1, \
 { 0x81, 0x45, 0xa0, 0xae, 0x57, 0xbc, 0xc7, 0x6e } }






class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  
  virtual PRUint32 GetScriptTypeID() = 0;

  
  
  
  virtual JSObject *GetScriptObject() = 0;

  
  virtual const PRUnichar *GetHandlerText() = 0;

  
  
  
  virtual void GetLocation(const char **aFileName, PRUint32 *aLineNo) = 0;

  
  
  virtual nsIArray *GetArgv() = 0;

  
  virtual PRUint32 GetScriptVersion() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
