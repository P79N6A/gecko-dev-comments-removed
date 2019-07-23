






































#ifndef _SHORTCUT_H_
#define _SHORTCUT_H_

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

#ifdef __cplusplus
extern "C"
{
#endif

long CreateALink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc, LPCSTR lpszWorkingPath, LPCSTR lpszArgs, LPCSTR lpszIconFullPath, int iIcon);

#ifdef __cplusplus
}
#endif

#endif
