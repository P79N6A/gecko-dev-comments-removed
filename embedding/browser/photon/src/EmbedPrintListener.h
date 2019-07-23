





































#ifndef __EmbedPrintListener_h
#define __EmbedPrintListener_h

#include <nsIWebProgressListener.h>

class EmbedPrivate;

class EmbedPrintListener : public nsIWebProgressListener
{
 public:

  EmbedPrintListener();
  virtual ~EmbedPrintListener();

  nsresult Init (EmbedPrivate *aOwner);

	void InvokePrintCallback(int, unsigned int, unsigned int);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWEBPROGRESSLISTENER

 private:

  EmbedPrivate *mOwner;
  
};

#endif 
