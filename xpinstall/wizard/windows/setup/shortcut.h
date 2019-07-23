






































#ifndef _SHORTCUT_H_
#define _SHORTCUT_H_

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

#ifdef __cplusplus
extern "C"
{
#endif

HRESULT CreateALink(LPSTR lpszPathObj, LPSTR lpszPathLink, LPSTR lpszDesc, LPSTR lpszWorkingPath, LPSTR lpszArgs, LPSTR lpszIconFullPath, int iIcon);

#ifdef __cplusplus
}
#endif

#endif