






























#include <string>

#include "common/linux/google_crashdump_uploader.h"
#include "common/linux/libcurl_wrapper.h"
#include "breakpad_googletest_includes.h"
#include "common/using_std_string.h"

namespace google_breakpad {

using ::testing::Return;
using ::testing::_;

class MockLibcurlWrapper : public LibcurlWrapper {
 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD2(SetProxy, bool(const string& proxy_host,
                              const string& proxy_userpwd));
  MOCK_METHOD2(AddFile, bool(const string& upload_file_path,
                             const string& basename));
  MOCK_METHOD3(SendRequest,
               bool(const string& url,
                    const std::map<string, string>& parameters,
                    string* server_response));
};

class GoogleCrashdumpUploaderTest : public ::testing::Test {
};

TEST_F(GoogleCrashdumpUploaderTest, InitFailsCausesUploadFailure) {
  MockLibcurlWrapper m;
  EXPECT_CALL(m, Init()).Times(1).WillOnce(Return(false));
  GoogleCrashdumpUploader *uploader = new GoogleCrashdumpUploader("foobar",
                                                                  "1.0",
                                                                  "AAA-BBB",
                                                                  "",
                                                                  "",
                                                                  "test@test.com",
                                                                  "none",
                                                                  "/tmp/foo.dmp",
                                                                  "http://foo.com",
                                                                  "",
                                                                  "",
                                                                  &m);
  ASSERT_FALSE(uploader->Upload());
}

TEST_F(GoogleCrashdumpUploaderTest, TestSendRequestHappensWithValidParameters) {
  
  char tempfn[80] = "/tmp/googletest-upload-XXXXXX";
  int fd = mkstemp(tempfn);
  ASSERT_NE(fd, -1);
  close(fd);

  MockLibcurlWrapper m;
  EXPECT_CALL(m, Init()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(m, AddFile(tempfn, _)).WillOnce(Return(true));
  EXPECT_CALL(m,
              SendRequest("http://foo.com",_,_)).Times(1).WillOnce(Return(true));
  GoogleCrashdumpUploader *uploader = new GoogleCrashdumpUploader("foobar",
                                                                  "1.0",
                                                                  "AAA-BBB",
                                                                  "",
                                                                  "",
                                                                  "test@test.com",
                                                                  "none",
                                                                  tempfn,
                                                                  "http://foo.com",
                                                                  "",
                                                                  "",
                                                                  &m);
  ASSERT_TRUE(uploader->Upload());
}


TEST_F(GoogleCrashdumpUploaderTest, InvalidPathname) {
  MockLibcurlWrapper m;
  EXPECT_CALL(m, Init()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(m, SendRequest(_,_,_)).Times(0);
  GoogleCrashdumpUploader *uploader = new GoogleCrashdumpUploader("foobar",
                                                                  "1.0",
                                                                  "AAA-BBB",
                                                                  "",
                                                                  "",
                                                                  "test@test.com",
                                                                  "none",
                                                                  "/tmp/foo.dmp",
                                                                  "http://foo.com",
                                                                  "",
                                                                  "",
                                                                  &m);
  ASSERT_FALSE(uploader->Upload());
}

TEST_F(GoogleCrashdumpUploaderTest, TestRequiredParametersMustBePresent) {
  
  GoogleCrashdumpUploader uploader("",
                                   "1.0",
                                   "AAA-BBB",
                                   "",
                                   "",
                                   "test@test.com",
                                   "none",
                                   "/tmp/foo.dmp",
                                   "http://foo.com",
                                   "",
                                   "");
  ASSERT_FALSE(uploader.Upload());

  
  GoogleCrashdumpUploader uploader1("product",
                                    "",
                                    "AAA-BBB",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "/tmp/foo.dmp",
                                    "",
                                    "",
                                    "");

  ASSERT_FALSE(uploader1.Upload());

  
  GoogleCrashdumpUploader uploader2("product",
                                    "1.0",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "",
                                    "/tmp/foo.dmp",
                                    "",
                                    "",
                                    "");
  ASSERT_FALSE(uploader2.Upload());
}
}
