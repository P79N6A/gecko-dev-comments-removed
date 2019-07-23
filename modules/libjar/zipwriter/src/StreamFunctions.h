






































#ifndef _nsStreamFunctions_h_
#define _nsStreamFunctions_h_

#include "nscore.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"







inline NS_HIDDEN_(void) WRITE8(char* buf, PRUint32* off, PRUint8 val)
{
  buf[(*off)++] = val & 0xff;
}

inline NS_HIDDEN_(void) WRITE16(char* buf, PRUint32* off, PRUint16 val)
{
  buf[(*off)++] = val & 0xff;
  buf[(*off)++] = (val >> 8) & 0xff;
}

inline NS_HIDDEN_(void) WRITE32(char* buf, PRUint32* off, PRUint32 val)
{
  buf[(*off)++] = val & 0xff;
  buf[(*off)++] = (val >> 8) & 0xff;
  buf[(*off)++] = (val >> 16) & 0xff;
  buf[(*off)++] = (val >> 24) & 0xff;
}

inline NS_HIDDEN_(PRUint8) READ8(char* buf, PRUint32* off)
{
  return (PRUint8)buf[(*off)++];
}

inline NS_HIDDEN_(PRUint16) READ16(char* buf, PRUint32* off)
{
  PRUint16 val = (PRUint16)buf[(*off)++] & 0xff;
  val |= ((PRUint16)buf[(*off)++] & 0xff) << 8;
  return val;
}

inline NS_HIDDEN_(PRUint32) READ32(char* buf, PRUint32* off)
{
  PRUint32 val = (PRUint32)buf[(*off)++] & 0xff;
  val |= ((PRUint32)buf[(*off)++] & 0xff) << 8;
  val |= ((PRUint32)buf[(*off)++] & 0xff) << 16;
  val |= ((PRUint32)buf[(*off)++] & 0xff) << 24;
  return val;
}

inline NS_HIDDEN_(PRUint32) PEEK32(unsigned char *buf)
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
