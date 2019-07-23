





































#ifndef nsObserverBase_h__
#define nsObserverBase_h__






class nsObserverBase {

public:

  nsObserverBase() {}
  virtual ~nsObserverBase() {}

  





protected:

  NS_IMETHOD NotifyDocShell(nsISupports* aDocShell,
                            nsISupports* aChannel,
                            const char* charset, 
                            PRInt32 source);

};

#endif 
