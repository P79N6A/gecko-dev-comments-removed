





























#ifndef MAILFRAME_H
#define MAILFRAME_H

#include "GeckoFrame.h"

class MailFrame :
    public GeckoFrame
{
public :
    MailFrame(wxWindow* aParent);

    DECLARE_EVENT_TABLE()

    void OnArticleClick(wxListEvent &event);

};

#endif