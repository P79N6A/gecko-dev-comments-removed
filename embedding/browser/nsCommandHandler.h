





#ifndef NSCOMMANDHANDLER_H
#define NSCOMMANDHANDLER_H

#include "nsISupports.h"
#include "nsICommandHandler.h"
#include "nsIDOMWindow.h"

class nsCommandHandler
  : public nsICommandHandlerInit
  , public nsICommandHandler
{
public:
  nsCommandHandler();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMMANDHANDLERINIT
  NS_DECL_NSICOMMANDHANDLER

protected:
  virtual ~nsCommandHandler();

private:
  nsresult GetCommandHandler(nsICommandHandler** aCommandHandler);

  nsIDOMWindow* mWindow;
};

#endif
