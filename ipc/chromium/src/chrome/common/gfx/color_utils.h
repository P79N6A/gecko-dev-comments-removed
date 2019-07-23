



#ifndef CHROME_COMMON_GFX_COLOR_UTILS_H__
#define CHROME_COMMON_GFX_COLOR_UTILS_H__

#include "SkColor.h"

class SkBitmap;

namespace color_utils {


struct CIE_XYZ {
  double X;
  double Y; 
  double Z;
};


struct LabColor {
  int L;
  int a;
  int b;
};




void SkColorToCIEXYZ(SkColor c, CIE_XYZ* xyz);
void CIEXYZToLabColor(const CIE_XYZ& xyz, LabColor* lab);

SkColor CIEXYZToSkColor(SkAlpha alpha, const CIE_XYZ& xyz);
void LabColorToCIEXYZ(const LabColor& lab, CIE_XYZ* xyz);

void SkColorToLabColor(SkColor c, LabColor* lab);
SkColor LabColorToSkColor(const LabColor& lab, SkAlpha alpha);


bool IsColorCloseToTransparent(SkAlpha alpha);


bool IsColorCloseToGrey(int r, int g, int b);




SkColor GetAverageColorOfFavicon(SkBitmap* bitmap, SkAlpha alpha);



void BuildLumaHistogram(SkBitmap* bitmap, int histogram[256]);


SkColor SetColorAlpha(SkColor c, SkAlpha alpha);


SkColor GetSysSkColor(int which);

} 

#endif  
