






































#ifndef __nsContentHandlerAppImpl_h__
#define __nsContentHandlerAppImpl_h__

#include <contentaction/contentaction.h>
#include "nsString.h"
#include "nsIMIMEInfo.h"

class nsContentHandlerApp : public nsIHandlerApp
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHANDLERAPP

  nsContentHandlerApp(nsString aName, nsCString aType, ContentAction::Action& aAction);
  virtual ~nsContentHandlerApp() { }

protected:
  nsString mName;
  nsCString mType;
  nsString mDetailedDescription;

  ContentAction::Action mAction;
};
#endif
