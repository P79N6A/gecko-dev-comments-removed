






































#ifndef nsRegionGTK_h___
#define nsRegionGTK_h___

#include "nsIRegion.h"

#include <gdk/gdk.h>

class nsRegionGTK : public nsIRegion
{
public:
  nsRegionGTK();
  virtual ~nsRegionGTK();

  NS_DECL_ISUPPORTS

  virtual nsresult Init();
#ifdef MOZ_WIDGET_GTK
  static void Shutdown();
#endif

  virtual void SetTo(const nsIRegion &aRegion);
  virtual void SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  virtual void Intersect(const nsIRegion &aRegion);
  virtual void Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  virtual void Union(const nsIRegion &aRegion);
  virtual void Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  virtual void Subtract(const nsIRegion &aRegion);
  virtual void Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  virtual PRBool IsEmpty(void);
  virtual PRBool IsEqual(const nsIRegion &aRegion);
  virtual void GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight);
  virtual void Offset(PRInt32 aXOffset, PRInt32 aYOffset);
  virtual PRBool ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD GetRects(nsRegionRectSet **aRects);
  NS_IMETHOD FreeRects(nsRegionRectSet *aRects);
  NS_IMETHOD GetNativeRegion(void *&aRegion) const;
  NS_IMETHOD GetRegionComplexity(nsRegionComplexity &aComplexity) const;
  NS_IMETHOD GetNumRects(PRUint32 *aRects) const;

protected:
#ifdef MOZ_WIDGET_GTK
  GdkRegion *gdk_region_copy(GdkRegion *region);
  GdkRegion *gdk_region_from_rect(PRInt32 aX, PRInt32 aY,
                                  PRInt32 aWidth, PRInt32 aHeight);

private:
  inline GdkRegion *GetCopyRegion();
  inline void SetRegionEmpty();

  static GdkRegion *copyRegion;
#endif
private:

  GdkRegion *mRegion;
};

#endif  
