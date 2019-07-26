









#include "webrtc/modules/desktop_capture/win/cursor.h"

#include <algorithm>

#include "webrtc/modules/desktop_capture/win/scoped_gdi_object.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

namespace {

#if defined(WEBRTC_ARCH_LITTLE_ENDIAN)

#define RGBA(r, g, b, a) \
    ((((a) << 24) & 0xff000000) | \
    (((b) << 16) & 0xff0000) | \
    (((g) << 8) & 0xff00) | \
    ((r) & 0xff))

#else  

#define RGBA(r, g, b, a) \
    ((((r) << 24) & 0xff000000) | \
    (((g) << 16) & 0xff0000) | \
    (((b) << 8) & 0xff00) | \
    ((a) & 0xff))

#endif  

const int kBytesPerPixel = DesktopFrame::kBytesPerPixel;


const uint32_t kPixelRgbaBlack = RGBA(0, 0, 0, 0xff);
const uint32_t kPixelRgbaWhite = RGBA(0xff, 0xff, 0xff, 0xff);
const uint32_t kPixelRgbaTransparent = RGBA(0, 0, 0, 0);

const uint32_t kPixelRgbWhite = RGB(0xff, 0xff, 0xff);
const uint32_t kPixelRgbBlack = RGB(0, 0, 0);



void AddCursorOutline(int width, int height, uint32_t* data) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      
      
      if (*data == kPixelRgbaTransparent) {
        
        
        if ((y > 0 && data[-width] == kPixelRgbaBlack) ||
            (y < height - 1 && data[width] == kPixelRgbaBlack) ||
            (x > 0 && data[-1] == kPixelRgbaBlack) ||
            (x < width - 1 && data[1] == kPixelRgbaBlack)) {
          *data = kPixelRgbaWhite;
        }
      }
      data++;
    }
  }
}


uint32_t AlphaMul(uint32_t pixel) {
  COMPILE_ASSERT(sizeof(uint32_t) == kBytesPerPixel);

  RGBQUAD from = *reinterpret_cast<RGBQUAD*>(&pixel);
  RGBQUAD to = {
    (static_cast<uint16_t>(from.rgbBlue) * from.rgbReserved) / 0xff,
    (static_cast<uint16_t>(from.rgbGreen) * from.rgbReserved) / 0xff,
    (static_cast<uint16_t>(from.rgbRed) * from.rgbReserved) / 0xff,
    from.rgbReserved
  };

  return *reinterpret_cast<uint32_t*>(&to);
}




bool HasAlphaChannel(const uint32_t* data, int stride, int width, int height,
                     bool* has_alpha) {
  const RGBQUAD* plane = reinterpret_cast<const RGBQUAD*>(data);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (plane->rgbReserved != 0) {
        *has_alpha = true;
        return true;
      }
      plane += 1;
    }
    plane += stride - width;
  }

  *has_alpha = false;
  return true;
}

}  

MouseCursorShape* CreateMouseCursorShapeFromCursor(HDC dc, HCURSOR cursor) {
  ICONINFO iinfo;
  if (!GetIconInfo(cursor, &iinfo)) {
    LOG_F(LS_ERROR) << "Unable to get cursor icon info. Error = "
                    << GetLastError();
    return NULL;
  }

  int hotspot_x = iinfo.xHotspot;
  int hotspot_y = iinfo.yHotspot;

  
  win::ScopedBitmap scoped_mask(iinfo.hbmMask);
  win::ScopedBitmap scoped_color(iinfo.hbmColor);
  bool is_color = iinfo.hbmColor != NULL;

  
  BITMAP bitmap_info;
  if (!GetObject(scoped_mask, sizeof(bitmap_info), &bitmap_info)) {
    LOG_F(LS_ERROR) << "Unable to get bitmap info. Error = "
                    << GetLastError();
    return NULL;
  }

  int width = bitmap_info.bmWidth;
  int height = bitmap_info.bmHeight;
  scoped_array<uint32_t> mask_data(new uint32_t[width * height]);

  
  
  BITMAPV5HEADER bmi = {0};
  bmi.bV5Size = sizeof(bmi);
  bmi.bV5Width = width;
  bmi.bV5Height = -height;  
  bmi.bV5Planes = 1;
  bmi.bV5BitCount = kBytesPerPixel * 8;
  bmi.bV5Compression = BI_RGB;
  bmi.bV5AlphaMask = 0xff000000;
  bmi.bV5CSType = LCS_WINDOWS_COLOR_SPACE;
  bmi.bV5Intent = LCS_GM_BUSINESS;
  if (!GetDIBits(dc,
                 scoped_mask,
                 0,
                 height,
                 mask_data.get(),
                 reinterpret_cast<BITMAPINFO*>(&bmi),
                 DIB_RGB_COLORS)) {
    LOG_F(LS_ERROR) << "Unable to get bitmap bits. Error = "
                    << GetLastError();
    return NULL;
  }

  uint32_t* mask_plane = mask_data.get();

  scoped_array<uint32_t> color_data;
  uint32_t* color_plane = NULL;
  int color_stride = 0;
  bool has_alpha = false;

  if (is_color) {
    
    color_data.reset(new uint32_t[width * height]);
    if (!GetDIBits(dc,
                   scoped_color,
                   0,
                   height,
                   color_data.get(),
                   reinterpret_cast<BITMAPINFO*>(&bmi),
                   DIB_RGB_COLORS)) {
      LOG_F(LS_ERROR) << "Unable to get bitmap bits. Error = "
                      << GetLastError();
      return NULL;
    }

    color_plane = color_data.get();
    color_stride = width;

    
    
    if (!HasAlphaChannel(color_plane, color_stride, width, height, &has_alpha))
      return NULL;
  } else {
    
    
    
    height /= 2;

    
    color_plane = mask_plane + (width * height);
    color_stride = width;
  }

  
  
  if (!has_alpha) {
    bool add_outline = false;
    uint32_t* color = color_plane;
    uint32_t* dst = color_plane;
    uint32_t* mask = mask_plane;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        
        
        
        
        
        
        
        
        
        
        if (*mask == kPixelRgbWhite) {
          if (*color != 0) {
            add_outline = true;
            *dst = kPixelRgbaBlack;
          } else {
            *dst = kPixelRgbaTransparent;
          }
        } else {
          *dst = kPixelRgbaBlack ^ *color;
        }

        ++color;
        ++dst;
        ++mask;
      }
    }
    if (add_outline) {
      AddCursorOutline(width, height, color_plane);
    }
  }

  scoped_ptr<MouseCursorShape> result(new MouseCursorShape());
  result->data.assign(reinterpret_cast<char*>(color_plane),
                      height * width * kBytesPerPixel);
  result->size.set(width, height);
  result->hotspot.set(hotspot_x, hotspot_y);
  return result.release();
}

}  
