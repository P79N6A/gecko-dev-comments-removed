





































#ifndef WINDOWCREATOR_H
#define WINDOWCREATOR_H

#include "nsIWindowCreator.h"
#include "nsIWebBrowserChrome.h"

class CWindowCreator : public nsIWindowCreator
{
public:
    CWindowCreator();
protected:
    virtual ~CWindowCreator();

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWCREATOR
};

#endif
