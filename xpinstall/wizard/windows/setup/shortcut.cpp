










































#if (_MSC_VER == 1100)
#define INITGUID
#include "objbase.h"
DEFINE_OLEGUID(IID_IPersistFile, 0x0000010BL, 0, 0);
#endif

#include <shlobj.h>
#include <shlguid.h>
#include "shortcut.h"

HRESULT CreateALink(LPSTR lpszPathObj, LPSTR lpszPathLink, LPSTR lpszDesc, LPSTR lpszWorkingPath, LPSTR lpszArgs, LPSTR lpszIconFullPath, int iIcon)
{ 
    HRESULT    hres; 
    IShellLink *psl;
    char       lpszFullPath[MAX_BUF];

    lstrcpy(lpszFullPath, lpszPathLink);
    lstrcat(lpszFullPath, "\\");
    lstrcat(lpszFullPath, lpszDesc);
    lstrcat(lpszFullPath, ".lnk");

    CreateDirectory(lpszPathLink, NULL);
    CoInitialize(NULL);

    
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl); 
    if(SUCCEEDED(hres))
    { 
        IPersistFile* ppf; 
 
        
        
        psl->SetPath(lpszPathObj);

        
        
        
        

        if(lpszWorkingPath)
            psl->SetWorkingDirectory(lpszWorkingPath);
        if(lpszArgs)
            psl->SetArguments(lpszArgs);
        psl->SetIconLocation(lpszIconFullPath, iIcon);
 
        
        
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID FAR *)&ppf); 
        if(SUCCEEDED(hres))
        { 
            WORD wsz[MAX_BUF]; 
 
            
            MultiByteToWideChar(CP_ACP, 0, lpszFullPath, -1, (wchar_t *)wsz, MAX_BUF);
 
            
            hres = ppf->Save((wchar_t *)wsz, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    CoUninitialize();

    return hres;
} 
