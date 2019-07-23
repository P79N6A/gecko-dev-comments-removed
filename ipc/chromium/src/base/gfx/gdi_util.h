



#ifndef BASE_GFX_GDI_UTIL_H__
#define BASE_GFX_GDI_UTIL_H__

#include <vector>
#include <windows.h>
#include "base/gfx/rect.h"

namespace gfx {


void CreateBitmapHeader(int width, int height, BITMAPINFOHEADER* hdr);



void CreateBitmapHeaderWithColorDepth(int width, int height, int color_depth,
                                      BITMAPINFOHEADER* hdr);




void CreateBitmapV4Header(int width, int height, BITMAPV4HEADER* hdr);


void CreateMonochromeBitmapHeader(int width, int height, BITMAPINFOHEADER* hdr);


void SubtractRectanglesFromRegion(HRGN hrgn,
                                  const std::vector<gfx::Rect>& cutouts);

}  

#endif 
