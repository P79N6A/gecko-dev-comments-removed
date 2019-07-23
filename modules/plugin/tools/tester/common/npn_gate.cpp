














































#ifdef __MWERKS__
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif

#ifdef __MWERKS__
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif






#include "xp.h"

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

  if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = (*NPNFuncs.geturlnotify) (instance, url, target, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  NPError rv = (*NPNFuncs.geturl) (instance, url, target);
  return rv;
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

	if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = (*NPNFuncs.posturlnotify) (instance, url, window, len, buf, file, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
  NPError rv = (*NPNFuncs.posturl) (instance, url, window, len, buf, file);
  return rv;
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  NPError rv = (*NPNFuncs.requestread) (stream, rangeList);
  return rv;
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = (*NPNFuncs.newstream) (instance, type, target, stream);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

int32_t NPN_Write(NPP instance, NPStream *stream, int32_t len, void *buffer)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  int32_t rv = 0;

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = (*NPNFuncs.write) (instance, stream, len, buffer);
	else
		rv = -1;

  return rv;
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = (*NPNFuncs.destroystream) (instance, stream, reason);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

void NPN_Status(NPP instance, const char *message)
{
  (*NPNFuncs.status) (instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  const char * rv = NULL;
  rv = (*NPNFuncs.uagent) (instance);
  return rv;
}

void* NPN_MemAlloc(uint32_t size)
{
  void * rv = NULL;
  rv = (*NPNFuncs.memalloc) (size);
  return rv;
}

void NPN_MemFree(void* ptr)
{
  (*NPNFuncs.memfree) (ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
  uint32_t rv = (*NPNFuncs.memflush) (size);
  return rv;
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  (*NPNFuncs.reloadplugins) (reloadPages);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  NPError rv = (*NPNFuncs.getvalue) (instance, variable, value);
  return rv;
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  NPError rv = (*NPNFuncs.setvalue) (instance, variable, value);
  return rv;
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  (*NPNFuncs.invalidaterect) (instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  (*NPNFuncs.invalidateregion) (instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  (*NPNFuncs.forceredraw) (instance);
}
