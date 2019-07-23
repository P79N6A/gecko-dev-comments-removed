









#ifndef BASE_GFX_NATIVE_THEME_H__
#define BASE_GFX_NATIVE_THEME_H__

#include <windows.h>
#include <uxtheme.h>
#include "base/basictypes.h"
#include "base/gfx/size.h"
#include "skia/include/SkColor.h"

namespace skia {
class PlatformCanvasWin;
}  

namespace gfx {








class NativeTheme {
 public:
  enum ThemeName {
    BUTTON,
    LIST,
    MENU,
    MENULIST,
    SCROLLBAR,
    STATUS,
    TAB,
    TEXTFIELD,
    TRACKBAR,
    WINDOW,
    LAST
  };

  
  
  enum MenuArrowDirection {
    LEFT_POINTING_ARROW,
    RIGHT_POINTING_ARROW
  };

  typedef HRESULT (WINAPI* DrawThemeBackgroundPtr)(HANDLE theme,
                                                   HDC hdc,
                                                   int part_id,
                                                   int state_id,
                                                   const RECT* rect,
                                                   const RECT* clip_rect);
  typedef HRESULT (WINAPI* DrawThemeBackgroundExPtr)(HANDLE theme,
                                                     HDC hdc,
                                                     int part_id,
                                                     int state_id,
                                                     const RECT* rect,
                                                     const DTBGOPTS* opts);
  typedef HRESULT (WINAPI* GetThemeColorPtr)(HANDLE hTheme,
                                             int part_id,
                                             int state_id,
                                             int prop_id,
                                             COLORREF* color);
  typedef HRESULT (WINAPI* GetThemeContentRectPtr)(HANDLE hTheme,
                                                   HDC hdc,
                                                   int part_id,
                                                   int state_id,
                                                   const RECT* rect,
                                                   RECT* content_rect);
  typedef HRESULT (WINAPI* GetThemePartSizePtr)(HANDLE hTheme,
                                                HDC hdc,
                                                int part_id,
                                                int state_id,
                                                RECT* rect,
                                                int ts,
                                                SIZE* size);
  typedef HANDLE (WINAPI* OpenThemeDataPtr)(HWND window,
                                            LPCWSTR class_list);
  typedef HRESULT (WINAPI* CloseThemeDataPtr)(HANDLE theme);

  typedef void (WINAPI* SetThemeAppPropertiesPtr) (DWORD flags);
  typedef BOOL (WINAPI* IsThemeActivePtr)();
  typedef HRESULT (WINAPI* GetThemeIntPtr)(HANDLE hTheme,
                                           int part_id,
                                           int state_id,
                                           int prop_id,
                                           int *value);

  HRESULT PaintButton(HDC hdc,
                      int part_id,
                      int state_id,
                      int classic_state,
                      RECT* rect) const;

  HRESULT PaintDialogBackground(HDC dc, bool active, RECT* rect) const;

  HRESULT PaintListBackground(HDC dc, bool enabled, RECT* rect) const;

  
  
  
  HRESULT PaintMenuArrow(ThemeName theme,
                         HDC hdc,
                         int part_id,
                         int state_id,
                         RECT* rect,
                         MenuArrowDirection arrow_direction,
                         bool is_highlighted) const;

  HRESULT PaintMenuBackground(ThemeName theme,
                              HDC hdc,
                              int part_id,
                              int state_id,
                              RECT* rect) const;

  HRESULT PaintMenuCheck(ThemeName theme,
                         HDC hdc,
                         int part_id,
                         int state_id,
                         RECT* rect,
                         bool is_highlighted) const;

  HRESULT PaintMenuCheckBackground(ThemeName theme,
                                   HDC hdc,
                                   int part_id,
                                   int state_id,
                                   RECT* rect) const;

  HRESULT PaintMenuGutter(HDC hdc,
                          int part_id,
                          int state_id,
                          RECT* rect) const;

  HRESULT PaintMenuItemBackground(ThemeName theme,
                                  HDC hdc,
                                  int part_id,
                                  int state_id,
                                  bool selected,
                                  RECT* rect) const;

  HRESULT PaintMenuList(HDC hdc,
                        int part_id,
                        int state_id,
                        int classic_state,
                        RECT* rect) const;

  HRESULT PaintMenuSeparator(HDC hdc,
                             int part_id,
                             int state_id,
                             RECT* rect) const;

  
  
  HRESULT PaintScrollbarArrow(HDC hdc,
                              int state_id,
                              int classic_state,
                              RECT* rect) const;

  
  
  
  HRESULT PaintScrollbarTrack(HDC hdc,
                              int part_id,
                              int state_id,
                              int classic_state,
                              RECT* target_rect,
                              RECT* align_rect,
                              skia::PlatformCanvasWin* canvas) const;

  
  HRESULT PaintScrollbarThumb(HDC hdc,
                              int part_id,
                              int state_id,
                              int classic_state,
                              RECT* rect) const;

  HRESULT PaintStatusGripper(HDC hdc,
                             int part_id,
                             int state_id,
                             int classic_state,
                             RECT* rect) const;

  HRESULT PaintTabPanelBackground(HDC dc, RECT* rect) const;

  HRESULT PaintTextField(HDC hdc,
                         int part_id,
                         int state_id,
                         int classic_state,
                         RECT* rect,
                         COLORREF color,
                         bool fill_content_area,
                         bool draw_edges) const;

  HRESULT PaintTrackbar(HDC hdc,
                        int part_id,
                        int state_id,
                        int classic_state,
                        RECT* rect,
                        skia::PlatformCanvasWin* canvas) const;

  bool IsThemingActive() const;

  HRESULT GetThemePartSize(ThemeName themeName,
                           HDC hdc,
                           int part_id,
                           int state_id,
                           RECT* rect,
                           int ts,
                           SIZE* size) const;

  HRESULT GetThemeColor(ThemeName theme,
                        int part_id,
                        int state_id,
                        int prop_id,
                        SkColor* color) const;

  
  
  
  SkColor GetThemeColorWithDefault(ThemeName theme,
                                   int part_id,
                                   int state_id,
                                   int prop_id,
                                   int default_sys_color) const;

  HRESULT GetThemeInt(ThemeName theme,
                      int part_id,
                      int state_id,
                      int prop_id,
                      int *result) const;

  
  
  
  Size GetThemeBorderSize(ThemeName theme) const;

  
  
  
  
  
  void DisableTheming() const;

  
  
  void CloseHandles() const;

  
  static const NativeTheme* instance();

 private:
  NativeTheme();
  ~NativeTheme();

  HRESULT PaintFrameControl(HDC hdc,
                            RECT* rect,
                            UINT type,
                            UINT state,
                            bool is_highlighted) const;

  
  HANDLE GetThemeHandle(ThemeName theme_name) const;

  
  DrawThemeBackgroundPtr draw_theme_;
  DrawThemeBackgroundExPtr draw_theme_ex_;
  GetThemeColorPtr get_theme_color_;
  GetThemeContentRectPtr get_theme_content_rect_;
  GetThemePartSizePtr get_theme_part_size_;
  OpenThemeDataPtr open_theme_;
  CloseThemeDataPtr close_theme_;
  SetThemeAppPropertiesPtr set_theme_properties_;
  IsThemeActivePtr is_theme_active_;
  GetThemeIntPtr get_theme_int_;

  
  HMODULE theme_dll_;

  
  mutable HANDLE theme_handles_[LAST];

  DISALLOW_EVIL_CONSTRUCTORS(NativeTheme);
};

}  

#endif
