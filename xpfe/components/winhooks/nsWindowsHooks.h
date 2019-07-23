





































#ifndef nswindowshooks_h____
#define nswindowshooks_h____

#include "nscore.h"
#include "nsIWindowsHooks.h"
#include "nsWindowsHooksUtil.h"

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif


#define NS_IWINDOWSHOOKS_CID \
 { 0xc09bc130, 0x0a71, 0x11d4, {0x80, 0x76, 0x00, 0x60, 0x08, 0x11, 0xa9, 0xc3} }

class nsWindowsHooksSettings : public nsIWindowsHooksSettings {
public:
    
    nsWindowsHooksSettings();
    virtual ~nsWindowsHooksSettings();

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWSHOOKSSETTINGS

    
    typedef
    NS_STDCALL_FUNCPROTO(nsresult,
                         getter,
                         nsIWindowsHooksSettings, GetShowDialog, 
                         (PRBool*));

    typedef
    NS_STDCALL_FUNCPROTO(nsresult,
                         setter,
                         nsIWindowsHooksSettings, SetShowDialog,
                         (PRBool));

protected:
    
    NS_IMETHOD Get( PRBool *result, PRBool nsWindowsHooksSettings::*member );
    
    NS_IMETHOD Set( PRBool value, PRBool nsWindowsHooksSettings::*member );

private:
    
        PRBool mHandleHTTP;
        PRBool mHandleHTTPS;
        PRBool mHandleFTP;
        PRBool mHandleCHROME;
        PRBool mHandleGOPHER;
    
        PRBool mHandleHTML;
        PRBool mHandleJPEG;
        PRBool mHandleGIF;
        PRBool mHandlePNG;
        PRBool mHandleMNG;
        PRBool mHandleXBM;
        PRBool mHandleBMP;
        PRBool mHandleICO;
        PRBool mHandleXML;
        PRBool mHandleXHTML;
        PRBool mHandleXUL;
    
        PRBool mShowDialog;

    
    PRBool mHaveBeenSet;
    NS_IMETHOD GetHaveBeenSet( PRBool * );
    NS_IMETHOD SetHaveBeenSet( PRBool );

    
    friend class nsWindowsHooks;
}; 

class nsWindowsHooks : public nsIWindowsHooks {
public:
    
    nsWindowsHooks();
    virtual ~nsWindowsHooks();

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWINDOWSHOOKS

protected:
    
    NS_IMETHOD GetSettings( nsWindowsHooksSettings ** );

    
    NS_IMETHOD SetRegistry();
    char mShortcutPath[MAX_BUF];
    char mShortcutName[MAX_BUF];
    char mShortcutBase[MAX_BUF];
    char mShortcutProg[MAX_BUF];

    
    ProtocolRegistryEntry http, https, ftp, chrome, gopher;
    FileTypeRegistryEntry jpg, gif, png, mng, xbm, bmp, ico, xml, xhtml, xul;
    EditableFileTypeRegistryEntry mozillaMarkup;

    
    friend class nsWindowsHooksSettings;
}; 

#endif 
