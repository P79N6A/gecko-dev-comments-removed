





































#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

class nsIArray;

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ /* {260C0DAB-0DCF-4c75-B820-46C31005718D} */ \
  0x260c0dab, 0xdcf, 0x4c75, \
  { 0xb8, 0x20, 0x46, 0xc3, 0x10, 0x5, 0x71, 0x8d } }






class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  
  virtual PRUint32 GetScriptTypeID() = 0;

  
  
  
  virtual void *GetScriptObject() = 0;

  
  virtual const PRUnichar *GetHandlerText() = 0;

  
  virtual void GetLocation(const char **aFileName, PRUint32 *aLineNo) = 0;

  
  
  virtual nsIArray *GetArgv() = 0;

  
  virtual PRUint32 GetScriptVersion() = 0;

  
  
  virtual void SetLateness(PRIntervalTime aHowLate) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif 
