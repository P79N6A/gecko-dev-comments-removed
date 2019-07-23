



#include "chrome/common/win_util.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(WinUtilTest, EnsureRectIsVisibleInRect) {
  gfx::Rect parent_rect(0, 0, 500, 400);

  {
    
    gfx::Rect child_rect(-50, 20, 100, 100);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(10, 20, 100, 100), child_rect);
  }

  {
    
    gfx::Rect child_rect(20, -50, 100, 100);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(20, 10, 100, 100), child_rect);
  }

  {
    
    gfx::Rect child_rect(450, 20, 100, 100);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(390, 20, 100, 100), child_rect);
  }

  {
    
    gfx::Rect child_rect(20, 350, 100, 100);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(20, 290, 100, 100), child_rect);
  }

  {
    
    gfx::Rect child_rect(20, 20, 700, 100);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(20, 20, 480, 100), child_rect);
  }

  {
    
    gfx::Rect child_rect(20, 20, 100, 700);
    win_util::EnsureRectIsVisibleInRect(parent_rect, &child_rect, 10);
    EXPECT_EQ(gfx::Rect(20, 20, 100, 380), child_rect);
  }
}

static const struct filename_case {
  const wchar_t* filename;
  const wchar_t* filter_selected;
  const wchar_t* suggested_ext;
  const wchar_t* result;
} filename_cases[] = {
  
  {L"f",         L"*.jpg", L"jpg", L"f.jpg"},
  {L"f.",        L"*.jpg", L"jpg", L"f..jpg"},
  {L"f..",       L"*.jpg", L"jpg", L"f...jpg"},
  {L"f.jpeg",    L"*.jpg", L"jpg", L"f.jpeg"},
  
  {L"f.jpg.jpg", L"*.jpg", L"jpg", L"f.jpg.jpg"},
  {L"f.exe.jpg", L"*.jpg", L"jpg", L"f.exe.jpg"},
  {L"f.jpg.exe", L"*.jpg", L"jpg", L"f.jpg.exe.jpg"},
  {L"f.exe..",   L"*.jpg", L"jpg", L"f.exe...jpg"},
  {L"f.jpg..",   L"*.jpg", L"jpg", L"f.jpg...jpg"},
  
  {L"f",         L"*.*",   L"jpg", L"f"},
  {L"f.",        L"*.*",   L"jpg", L"f"},
  {L"f..",       L"*.*",   L"jpg", L"f"},
  {L"f.jpg",     L"*.*",   L"jpg", L"f.jpg"},
  {L"f.jpeg",    L"*.*",   L"jpg", L"f.jpeg"},  
  
  
  {L"f",         L"",      L"jpg", L"f"},
  {L"f.",        L"",      L"jpg", L"f"},
  {L"f..",       L"",      L"jpg", L"f"},
  {L"f.jpg",     L"",      L"jpg", L"f.jpg"},
  {L"f.jpeg",    L"",      L"jpg", L"f.jpeg"},
};

TEST(WinUtilTest, AppendingExtensions) {
  for (unsigned int i = 0; i < arraysize(filename_cases); ++i) {
    const filename_case& value = filename_cases[i];
    std::wstring result =
        win_util::AppendExtensionIfNeeded(value.filename, value.filter_selected,
                                          value.suggested_ext);
    EXPECT_EQ(value.result, result);
  }
}
