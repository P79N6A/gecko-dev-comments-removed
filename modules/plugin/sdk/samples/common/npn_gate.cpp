






































#include "npplat.h"

extern NPNetscapeFuncs NPNFuncs;

void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
  *plugin_major   = NP_VERSION_MAJOR;
  *plugin_minor   = NP_VERSION_MINOR;
  *netscape_major = HIBYTE(NPNFuncs.version);
  *netscape_minor = LOBYTE(NPNFuncs.version);
}

NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

  if (navMinorVers >= NPVERS_HAS_NOTIFICATION)
		rv = CallNPN_GetURLNotifyProc(NPNFuncs.geturlnotify, instance, url, target, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  return CallNPN_GetURLProc(NPNFuncs.geturl, instance, url, target);
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

	if (navMinorVers >= NPVERS_HAS_NOTIFICATION)
		rv = CallNPN_PostURLNotifyProc(NPNFuncs.posturlnotify, instance, url, window, len, buf, file, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
  return CallNPN_PostURLProc(NPNFuncs.posturl, instance, url, window, len, buf, file);
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  return CallNPN_RequestReadProc(NPNFuncs.requestread, stream, rangeList);
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

	if (navMinorVersion >= NPVERS_HAS_STREAMOUTPUT)
		rv = CallNPN_NewStreamProc(NPNFuncs.newstream, instance, type, target, stream);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

int32_t NPN_Write(NPP instance, NPStream *stream, int32_t len, void *buffer)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  int32_t rv = 0;

  if (navMinorVersion >= NPVERS_HAS_STREAMOUTPUT)
		rv = CallNPN_WriteProc(NPNFuncs.write, instance, stream, len, buffer);
	else
		rv = -1;

  return rv;
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

  if (navMinorVersion >= NPVERS_HAS_STREAMOUTPUT)
		rv = CallNPN_DestroyStreamProc(NPNFuncs.destroystream, instance, stream, reason);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

void NPN_Status(NPP instance, const char *message)
{
  CallNPN_StatusProc(NPNFuncs.status, instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  return CallNPN_UserAgentProc(NPNFuncs.uagent, instance);
}

void* NPN_MemAlloc(uint32_t size)
{
  return CallNPN_MemAllocProc(NPNFuncs.memalloc, size);
}

void NPN_MemFree(void* ptr)
{
  CallNPN_MemFreeProc(NPNFuncs.memfree, ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
  return CallNPN_MemFlushProc(NPNFuncs.memflush, size);
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  CallNPN_ReloadPluginsProc(NPNFuncs.reloadplugins, reloadPages);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  return CallNPN_GetValueProc(NPNFuncs.getvalue, instance, variable, value);
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  return CallNPN_SetValueProc(NPNFuncs.setvalue, instance, variable, value);
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  CallNPN_InvalidateRectProc(NPNFuncs.invalidaterect, instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  CallNPN_InvalidateRegionProc(NPNFuncs.invalidateregion, instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  CallNPN_ForceRedrawProc(NPNFuncs.forceredraw, instance);
}
