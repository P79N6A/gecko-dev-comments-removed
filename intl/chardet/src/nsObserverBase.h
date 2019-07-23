





































#ifndef nsObserverBase_h__
#define nsObserverBase_h__






class nsObserverBase {

public:

  nsObserverBase() {}
  virtual ~nsObserverBase() {}

  





protected:

  NS_IMETHOD NotifyWebShell(nsISupports* aWebShell,
                            nsISupports* aChannel,
                            const char* charset, 
                            PRInt32 source);

};

#endif 
