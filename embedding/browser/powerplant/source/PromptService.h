




































#ifndef __PromptService_h
#define __PromptService_h

#include "nsIPromptService.h"





class CPromptService: public nsIPromptService
{
public:
			CPromptService();
  virtual	~CPromptService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROMPTSERVICE
};

#endif
