





































#ifndef __EmbedWindowCreator_h
#define __EmbedWindowCreator_h

#include <nsIWindowCreator.h>

class EmbedWindowCreator : public nsIWindowCreator
{
 public:
  EmbedWindowCreator();
  virtual ~EmbedWindowCreator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWCREATOR
  
};

#endif
