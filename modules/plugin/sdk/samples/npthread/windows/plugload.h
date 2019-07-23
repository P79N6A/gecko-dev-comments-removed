




































#ifndef __PLUGLOAD_H__
#define __PLUGLOAD_H__

DWORD GetPluginsDir(char * path, DWORD maxsize);
HINSTANCE LoadRealPlugin(char * mimetype);
void UnloadRealPlugin(HINSTANCE hLib);

#endif
