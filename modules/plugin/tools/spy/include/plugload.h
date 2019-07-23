




































#ifndef __PLUGLOAD_H__
#define __PLUGLOAD_H__

DWORD GetPluginsDir(char * path, DWORD maxsize);
XP_HLIB LoadRealPlugin(char * mimetype);
void UnloadRealPlugin(XP_HLIB hLib);

#endif
