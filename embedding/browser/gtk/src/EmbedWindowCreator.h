






































#ifndef __EmbedWindowCreator_h
#define __EmbedWindowCreator_h

#include "nsIWindowCreator.h"

class EmbedWindowCreator : public nsIWindowCreator
{
 public:
  EmbedWindowCreator(PRBool *aOpenBlockPtr);
  virtual ~EmbedWindowCreator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWCREATOR

 private:
  PRBool *mOpenBlock;
};

#endif 
