





































#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

class nsIArray;

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ /* {21ba4f96-30b8-4215-a75d-d438eb16a50c} */ \
  0x21ba4f96, 0x30b8, 0x4215, \
  { 0xa7, 0x5d, 0xd4, 0x38, 0xeb, 0x16, 0xa5, 0x0c } }






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

  
  
  virtual void SetLateness(PRIntervalTime aHowLate) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
