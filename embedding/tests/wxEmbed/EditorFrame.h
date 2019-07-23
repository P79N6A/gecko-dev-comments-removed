





























#ifndef EDITORFRAME_H
#define EDITORFRAME_H

#include "GeckoFrame.h"

#include "nsICommandManager.h"
#include "nsICommandParams.h"

class EditorFrame :
    public GeckoFrame
{
protected:
    DECLARE_EVENT_TABLE()
    void OnEditBold(wxCommandEvent &event);
    void OnEditItalic(wxCommandEvent &event);
    void OnEditUnderline(wxCommandEvent &event);
    void OnEditIndent(wxCommandEvent &event);
    void OnEditOutdent(wxCommandEvent &event);
    void OnUpdateToggleCmd(wxUpdateUIEvent &event);
    void OnEditAlignLeft(wxCommandEvent &event);
    void OnEditAlignRight(wxCommandEvent &event);
    void OnEditAlignCenter(wxCommandEvent &event);
public :
    EditorFrame(wxWindow* aParent);

    nsCOMPtr<nsICommandManager> mCommandManager;

    
    virtual void UpdateStatusBarText(const PRUnichar* aStatusText);

    void MakeEditable();
    nsresult DoCommand(const char *aCommand, nsICommandParams *aCommandParams);
    void IsCommandEnabled(const char *aCommand, PRBool *retval);
    void GetCommandState(const char *aCommand, nsICommandParams *aCommandParams);
    nsresult ExecuteAttribParam(const char *aCommand, const char *aAttribute);
};

#endif