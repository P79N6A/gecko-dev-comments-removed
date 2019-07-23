






































#ifndef nsXFont_h__
#define nsXFont_h__

class nsXFont {
public:
  virtual ~nsXFont() { };
  virtual void         DrawText8(GdkDrawable *Drawable, GdkGC *GC, PRInt32,
                                 PRInt32, const char *, PRUint32) = 0;
  virtual void         DrawText16(GdkDrawable *Drawable, GdkGC *GC, PRInt32,
                                  PRInt32, const XChar2b *, PRUint32) = 0;
  virtual PRBool       GetXFontProperty(Atom, unsigned long *) = 0;
  virtual XFontStruct *GetXFontStruct() = 0;
  inline  PRBool       IsSingleByte() { return mIsSingleByte; };
  virtual PRBool       LoadFont() = 0;
  virtual void         TextExtents8(const char *, PRUint32, PRInt32*, PRInt32*,
                                   PRInt32*, PRInt32*, PRInt32*) = 0;
  virtual void         TextExtents16(const XChar2b *, PRUint32, PRInt32*,
                                     PRInt32*, PRInt32*, PRInt32*, PRInt32*) =0;
  virtual PRInt32      TextWidth8(const char *, PRUint32) = 0;
  virtual PRInt32      TextWidth16(const XChar2b *, PRUint32) = 0;

  virtual void         UnloadFont() = 0;
protected:
  PRBool mIsSingleByte;
};

#endif 
