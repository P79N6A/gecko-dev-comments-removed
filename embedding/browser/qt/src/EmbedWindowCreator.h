






































#ifndef EMBEDWINDOWCREATOR_H
#define EMBEDWINDOWCREATOR_H

#include <nsIWindowCreator.h>

class EmbedWindowCreator : public nsIWindowCreator
{
public:
    EmbedWindowCreator();
    ~EmbedWindowCreator();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWCREATOR
};

#endif
