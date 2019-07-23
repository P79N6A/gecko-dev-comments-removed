





























#ifndef GECKOWND_H_270B94BB_E0EE_44bd_8983_CC43BFA39996
#define GECKOWND_H_270B94BB_E0EE_44bd_8983_CC43BFA39996

#include "GeckoContainer.h"

class GeckoWindow : public wxPanel
{
    DECLARE_DYNAMIC_CLASS(GeckoWindow);

protected:
    DECLARE_EVENT_TABLE()
    void OnSize(wxSizeEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnKillFocus(wxFocusEvent &event);

    GeckoContainer *mGeckoContainer;

public:
    GeckoWindow();
    virtual ~GeckoWindow();
    void SetGeckoContainer(GeckoContainer *aGeckoContainer);
    GeckoContainer *GetGeckoContainer() const { return mGeckoContainer; }
};

#endif 