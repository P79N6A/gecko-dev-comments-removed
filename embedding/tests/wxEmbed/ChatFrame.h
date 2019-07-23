





























#ifndef CHATFRAME_H

#include "GeckoFrame.h"

class ChatFrame :
    public GeckoFrame
{
public :
    ChatFrame(wxWindow* aParent);

    DECLARE_EVENT_TABLE()

    void OnChat(wxCommandEvent &event);
};

#endif