




































#ifndef nsIReflowCallback_h___
#define nsIReflowCallback_h___











class nsIReflowCallback {
public:
  



  virtual PRBool ReflowFinished() = 0;
  




  virtual void ReflowCallbackCanceled() = 0;
};

#endif 
