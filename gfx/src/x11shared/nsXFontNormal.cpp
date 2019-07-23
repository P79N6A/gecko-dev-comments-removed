






































#include "gfx-config.h"
#include "nscore.h"
#include "nsXFontNormal.h"
#include "nsRenderingContextGTK.h"
#include "nsGdkUtils.h"

void
nsXFontNormal::DrawText8(GdkDrawable *aDrawable, GdkGC *aGC,
                         PRInt32 aX, PRInt32 aY,
                        const char *aString, PRUint32 aLength)
{
  my_gdk_draw_text(aDrawable, mGdkFont, aGC, aX, aY, aString, aLength);
}

void
nsXFontNormal::DrawText16(GdkDrawable *aDrawable, GdkGC *aGC,
                         PRInt32 aX, PRInt32 aY,
                        const XChar2b *aString, PRUint32 aLength)
{
  my_gdk_draw_text(aDrawable, mGdkFont, aGC, aX, aY,
                   (const char *)aString, aLength*2);
}

PRBool
nsXFontNormal::GetXFontProperty(Atom aAtom, unsigned long *aValue)
{
  NS_ASSERTION(mGdkFont, "GetXFontProperty called before font loaded");
  if (mGdkFont==nsnull)
    return PR_FALSE;

  XFontStruct *fontInfo = (XFontStruct *)GDK_FONT_XFONT(mGdkFont);

  return ::XGetFontProperty(fontInfo, aAtom, aValue);
}

XFontStruct *
nsXFontNormal::GetXFontStruct()
{
  NS_ASSERTION(mGdkFont, "GetXFontStruct called before font loaded");
  if (mGdkFont==nsnull)
    return nsnull;

  return (XFontStruct *)GDK_FONT_XFONT(mGdkFont);
}

PRBool
nsXFontNormal::LoadFont()
{
  if (!mGdkFont)
    return PR_FALSE;
  XFontStruct *fontInfo = (XFontStruct *)GDK_FONT_XFONT(mGdkFont);
  mIsSingleByte = (fontInfo->min_byte1 == 0) && (fontInfo->max_byte1 == 0);
  return PR_TRUE;
}

nsXFontNormal::nsXFontNormal(GdkFont *aGdkFont)
{
  mGdkFont = ::gdk_font_ref(aGdkFont);
}

void
nsXFontNormal::TextExtents8(const char *aString, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  gdk_text_extents(mGdkFont, aString, aLength,
                    aLBearing, aRBearing, aWidth, aAscent, aDescent);
}

void
nsXFontNormal::TextExtents16(const XChar2b *aString, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  gdk_text_extents(mGdkFont, (const char *)aString, aLength*2,
                    aLBearing, aRBearing, aWidth, aAscent, aDescent);
}

PRInt32
nsXFontNormal::TextWidth8(const char *aString, PRUint32 aLength)
{
  NS_ASSERTION(mGdkFont, "TextWidth8 called before font loaded");
  if (mGdkFont==nsnull)
    return 0;
  PRInt32 width = gdk_text_width(mGdkFont, aString, aLength);
  return width;
}

PRInt32
nsXFontNormal::TextWidth16(const XChar2b *aString, PRUint32 aLength)
{
  NS_ASSERTION(mGdkFont, "TextWidth16 called before font loaded");
  if (mGdkFont==nsnull)
    return 0;
  PRInt32 width = gdk_text_width(mGdkFont, (const char *)aString, aLength*2);
  return width;
}

void
nsXFontNormal::UnloadFont()
{
  delete this;
}

nsXFontNormal::~nsXFontNormal()
{
  if (mGdkFont) {
    ::gdk_font_unref(mGdkFont);
  }
}

