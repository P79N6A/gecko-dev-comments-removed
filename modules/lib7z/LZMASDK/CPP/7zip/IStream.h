

#ifndef __ISTREAM_H
#define __ISTREAM_H

#include "../Common/MyUnknown.h"
#include "../Common/Types.h"

#include "IDecl.h"

#define STREAM_INTERFACE_SUB(i, base, x) DECL_INTERFACE_SUB(i, base, 3, x)
#define STREAM_INTERFACE(i, x) STREAM_INTERFACE_SUB(i, IUnknown, x)

STREAM_INTERFACE(ISequentialInStream, 0x01)
{
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize) PURE;
  







};

STREAM_INTERFACE(ISequentialOutStream, 0x02)
{
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize) PURE;
  




};

STREAM_INTERFACE_SUB(IInStream, ISequentialInStream, 0x03)
{
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) PURE;
};

STREAM_INTERFACE_SUB(IOutStream, ISequentialOutStream, 0x04)
{
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) PURE;
  STDMETHOD(SetSize)(Int64 newSize) PURE;
};

STREAM_INTERFACE(IStreamGetSize, 0x06)
{
  STDMETHOD(GetSize)(UInt64 *size) PURE;
};

STREAM_INTERFACE(IOutStreamFlush, 0x07)
{
  STDMETHOD(Flush)() PURE;
};

#endif
