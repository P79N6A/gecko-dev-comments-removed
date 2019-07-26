









#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/modules/desktop_capture/mouse_cursor.h"
#include "webrtc/modules/desktop_capture/win/cursor.h"
#include "webrtc/modules/desktop_capture/win/cursor_unittest_resources.h"
#include "webrtc/modules/desktop_capture/win/scoped_gdi_object.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

namespace {




bool ConvertToMouseShapeAndCompare(unsigned left, unsigned right) {
  HMODULE instance = GetModuleHandle(NULL);

  
  win::ScopedCursor cursor(reinterpret_cast<HCURSOR>(
      LoadImage(instance, MAKEINTRESOURCE(left), IMAGE_CURSOR, 0, 0, 0)));
  EXPECT_TRUE(cursor != NULL);

  
  HDC dc = GetDC(NULL);
  scoped_ptr<MouseCursor> mouse_shape(
      CreateMouseCursorFromHCursor(dc, cursor));
  ReleaseDC(NULL, dc);

  EXPECT_TRUE(mouse_shape.get());

  
  cursor.Set(reinterpret_cast<HCURSOR>(
      LoadImage(instance, MAKEINTRESOURCE(right), IMAGE_CURSOR, 0, 0, 0)));

  ICONINFO iinfo;
  EXPECT_TRUE(GetIconInfo(cursor, &iinfo));
  EXPECT_TRUE(iinfo.hbmColor);

  
  win::ScopedBitmap scoped_mask(iinfo.hbmMask);
  win::ScopedBitmap scoped_color(iinfo.hbmColor);

  
  BITMAP bitmap_info;
  EXPECT_TRUE(GetObject(scoped_color, sizeof(bitmap_info), &bitmap_info));

  int width = bitmap_info.bmWidth;
  int height = bitmap_info.bmHeight;
  EXPECT_TRUE(DesktopSize(width, height).equals(mouse_shape->image()->size()));

  
  int size = width * height;
  scoped_array<uint32_t> data(new uint32_t[size]);
  EXPECT_TRUE(GetBitmapBits(scoped_color, size * sizeof(uint32_t), data.get()));

  
  return memcmp(data.get(), mouse_shape->image()->data(),
                size * sizeof(uint32_t)) == 0;
}

}  

TEST(MouseCursorTest, MatchCursors) {
  EXPECT_TRUE(ConvertToMouseShapeAndCompare(IDD_CURSOR1_24BPP,
                                            IDD_CURSOR1_32BPP));

  EXPECT_TRUE(ConvertToMouseShapeAndCompare(IDD_CURSOR1_8BPP,
                                            IDD_CURSOR1_32BPP));

  EXPECT_TRUE(ConvertToMouseShapeAndCompare(IDD_CURSOR2_1BPP,
                                            IDD_CURSOR2_32BPP));

  EXPECT_TRUE(ConvertToMouseShapeAndCompare(IDD_CURSOR3_4BPP,
                                            IDD_CURSOR3_32BPP));
}

}  
