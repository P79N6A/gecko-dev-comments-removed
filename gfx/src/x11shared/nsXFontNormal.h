






































#ifndef nsXFontNormal_h__
#define nsXFontNormal_h__

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include "nspr.h"
#include "nsXFont.h"

class nsXFontNormal : public nsXFont {
public:
  nsXFontNormal(GdkFont *);
  ~nsXFontNormal();

  void         DrawText8(GdkDrawable *Drawable, GdkGC *GC, PRInt32, PRInt32,
                         const char *, PRUint32);
  void         DrawText16(GdkDrawable *Drawable, GdkGC *GC, PRInt32, PRInt32,
                          const XChar2b *, PRUint32);
  PRBool       GetXFontProperty(Atom, unsigned long *);
  XFontStruct *GetXFontStruct();
  PRBool       LoadFont();
  void         TextExtents8(const char *, PRUint32, PRInt32*, PRInt32*,
                            PRInt32*, PRInt32*, PRInt32*);
  void         TextExtents16(const XChar2b *, PRUint32, PRInt32*, PRInt32*,
                             PRInt32*, PRInt32*, PRInt32*);
  PRInt32      TextWidth8(const char *, PRUint32);
  PRInt32      TextWidth16(const XChar2b *, PRUint32);
  void         UnloadFont();
protected:
  GdkFont *mGdkFont;
};

#endif 
