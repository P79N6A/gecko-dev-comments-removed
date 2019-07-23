






































#ifndef _nsStreamFunctions_h_
#define _nsStreamFunctions_h_

#include "nscore.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"







inline NS_HIDDEN_(void) WRITE8(PRUint8* buf, PRUint32* off, PRUint8 val)
{
  buf[(*off)++] = val;
}

inline NS_HIDDEN_(void) WRITE16(PRUint8* buf, PRUint32* off, PRUint16 val)
{
  WRITE8(buf, off, val & 0xff);
  WRITE8(buf, off, (val >> 8) & 0xff);
}

inline NS_HIDDEN_(void) WRITE32(PRUint8* buf, PRUint32* off, PRUint32 val)
{
  WRITE16(buf, off, val & 0xffff);
  WRITE16(buf, off, (val >> 16) & 0xffff);
}

inline NS_HIDDEN_(PRUint8) READ8(const PRUint8* buf, PRUint32* off)
{
  return buf[(*off)++];
}

inline NS_HIDDEN_(PRUint16) READ16(const PRUint8* buf, PRUint32* off)
{
  PRUint16 val = READ8(buf, off);
  val |= READ8(buf, off) << 8;
  return val;
}

inline NS_HIDDEN_(PRUint32) READ32(const PRUint8* buf, PRUint32* off)
{
  PRUint32 val = READ16(buf, off);
  val |= READ16(buf, off) << 16;
  return val;
}

inline NS_HIDDEN_(PRUint32) PEEK32(const PRUint8* buf)
{
  return (PRUint32)( (buf [0]      ) |
                     (buf [1] <<  8) |
                     (buf [2] << 16) |
                     (buf [3] << 24) );
}

NS_HIDDEN_(nsresult) ZW_ReadData(nsIInputStream *aStream, char *aBuffer, PRUint32 aCount);

NS_HIDDEN_(nsresult) ZW_WriteData(nsIOutputStream *aStream, const char *aBuffer,
                      PRUint32 aCount);

#endif
