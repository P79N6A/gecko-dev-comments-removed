



#include "chrome/common/gfx/emf.h"


#include <wingdi.h>

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "chrome/browser/printing/win_printing_context.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {



class EmfPrintingTest : public testing::Test {
 public:
  typedef testing::Test Parent;
  static bool IsTestCaseDisabled() {
    
    HDC hdc = CreateDC(L"WINSPOOL", L"UnitTest Printer", NULL, NULL);
    if (!hdc)
      return true;
    DeleteDC(hdc);
    return false;
  }
};

}  

TEST(EmfTest, DC) {
  static const int EMF_HEADER_SIZE = 128;

  
  gfx::Emf emf;
  RECT rect = {100, 100, 200, 200};
  HDC hdc = CreateCompatibleDC(NULL);
  EXPECT_TRUE(hdc != NULL);
  EXPECT_TRUE(emf.CreateDc(hdc, &rect));
  EXPECT_TRUE(emf.hdc() != NULL);
  
  EXPECT_TRUE(emf.CloseDc());
  unsigned size = emf.GetDataSize();
  EXPECT_EQ(size, EMF_HEADER_SIZE);
  std::vector<BYTE> data;
  EXPECT_TRUE(emf.GetData(&data));
  EXPECT_EQ(data.size(), size);
  emf.CloseEmf();
  EXPECT_TRUE(DeleteDC(hdc));

  
  hdc = CreateCompatibleDC(NULL);
  EXPECT_TRUE(hdc);
  EXPECT_TRUE(emf.CreateFromData(&data.front(), size));
  RECT output_rect = {0, 0, 10, 10};
  EXPECT_TRUE(emf.Playback(hdc, &output_rect));
  EXPECT_TRUE(DeleteDC(hdc));
}



TEST_F(EmfPrintingTest, Enumerate) {
  if (IsTestCaseDisabled())
    return;

  printing::PrintSettings settings;

  
  settings.set_device_name(L"UnitTest Printer");

  
  printing::PrintingContext context;
  EXPECT_EQ(context.InitWithSettings(settings), printing::PrintingContext::OK);

  std::wstring test_file;
  PathService::Get(chrome::DIR_TEST_DATA, &test_file);

  
  gfx::Emf emf;
  file_util::AppendToPath(&test_file, L"printing");
  file_util::AppendToPath(&test_file, L"test4.emf");
  std::string emf_data;
  file_util::ReadFileToString(test_file, &emf_data);
  ASSERT_TRUE(emf_data.size());
  EXPECT_TRUE(emf.CreateFromData(&emf_data[0], emf_data.size()));

  
  
  
  
  context.NewDocument(L"EmfTest.Enumerate");
  context.NewPage();
  
  gfx::Emf::Enumerator emf_enum(emf, context.context(),
                                &emf.GetBounds().ToRECT());
  for (gfx::Emf::Enumerator::const_iterator itr = emf_enum.begin();
       itr != emf_enum.end();
       ++itr) {
    
    ptrdiff_t index = itr - emf_enum.begin();
    
    
    EMR_HEADER;
    EXPECT_TRUE(itr->SafePlayback(NULL)) <<
        " index: " << index << " type: " << itr->record()->iType;
  }
  context.PageDone();
  context.DocumentDone();
}
