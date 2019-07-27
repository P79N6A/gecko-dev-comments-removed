









#include <string>

#include "webrtc/base/fileutils_mock.h"
#include "webrtc/base/proxydetect.h"

namespace rtc {

static const std::string kFirefoxProfilesIni =
  "[Profile0]\n"
  "Name=default\n"
  "IsRelative=1\n"
  "Path=Profiles/2de53ejb.default\n"
  "Default=1\n";

static const std::string kFirefoxHeader =
  "# Mozilla User Preferences\n"
  "\n"
  "/* Some Comments\n"
  "*\n"
  "*/\n"
  "\n";

static const std::string kFirefoxCorruptHeader =
  "iuahueqe32164";

static const std::string kProxyAddress = "proxy.net.com";


class FirefoxPrefsFileSystem : public FakeFileSystem {
 public:
  explicit FirefoxPrefsFileSystem(const std::vector<File>& all_files) :
      FakeFileSystem(all_files) {
  }
  virtual FileStream* OpenFile(const Pathname& filename,
                               const std::string& mode) {
    
    std::string name = filename.basename();
    name.append(filename.extension());
    EXPECT_TRUE(name.compare("prefs.js") == 0 ||
                name.compare("profiles.ini") == 0);
    FileStream* stream = FakeFileSystem::OpenFile(name, mode);
    return stream;
  }
};

class ProxyDetectTest : public testing::Test {
};

bool GetProxyInfo(const std::string prefs, ProxyInfo* info) {
  std::vector<rtc::FakeFileSystem::File> files;
  files.push_back(rtc::FakeFileSystem::File("profiles.ini",
                                                  kFirefoxProfilesIni));
  files.push_back(rtc::FakeFileSystem::File("prefs.js", prefs));
  rtc::FilesystemScope fs(new rtc::FirefoxPrefsFileSystem(files));
  return GetProxySettingsForUrl("Firefox", "www.google.com", info, false);
}


TEST_F(ProxyDetectTest, DISABLED_TestFirefoxEmptyPrefs) {
  ProxyInfo proxy_info;
  EXPECT_TRUE(GetProxyInfo(kFirefoxHeader, &proxy_info));
  EXPECT_EQ(PROXY_NONE, proxy_info.type);
}


TEST_F(ProxyDetectTest, DISABLED_TestFirefoxCorruptedPrefs) {
  ProxyInfo proxy_info;
  EXPECT_TRUE(GetProxyInfo(kFirefoxCorruptHeader, &proxy_info));
  EXPECT_EQ(PROXY_NONE, proxy_info.type);
}





TEST_F(ProxyDetectTest, DISABLED_TestFirefoxProxySocks) {
  ProxyInfo proxy_info;
  SocketAddress proxy_address("proxy.socks.com", 6666);
  std::string prefs(kFirefoxHeader);
  prefs.append("user_pref(\"network.proxy.socks\", \"proxy.socks.com\");\n");
  prefs.append("user_pref(\"network.proxy.socks_port\", 6666);\n");
  prefs.append("user_pref(\"network.proxy.type\", 1);\n");

  EXPECT_TRUE(GetProxyInfo(prefs, &proxy_info));

  EXPECT_EQ(PROXY_SOCKS5, proxy_info.type);
  EXPECT_EQ(proxy_address, proxy_info.address);
}



TEST_F(ProxyDetectTest, DISABLED_TestFirefoxProxySsl) {
  ProxyInfo proxy_info;
  SocketAddress proxy_address("proxy.ssl.com", 7777);
  std::string prefs(kFirefoxHeader);

  prefs.append("user_pref(\"network.proxy.ssl\", \"proxy.ssl.com\");\n");
  prefs.append("user_pref(\"network.proxy.ssl_port\", 7777);\n");
  prefs.append("user_pref(\"network.proxy.type\", 1);\n");

  EXPECT_TRUE(GetProxyInfo(prefs, &proxy_info));

  EXPECT_EQ(PROXY_HTTPS, proxy_info.type);
  EXPECT_EQ(proxy_address, proxy_info.address);
}


TEST_F(ProxyDetectTest, DISABLED_TestFirefoxProxyHttp) {
  ProxyInfo proxy_info;
  SocketAddress proxy_address("proxy.http.com", 8888);
  std::string prefs(kFirefoxHeader);

  prefs.append("user_pref(\"network.proxy.http\", \"proxy.http.com\");\n");
  prefs.append("user_pref(\"network.proxy.http_port\", 8888);\n");
  prefs.append("user_pref(\"network.proxy.type\", 1);\n");

  EXPECT_TRUE(GetProxyInfo(prefs, &proxy_info));

  EXPECT_EQ(PROXY_HTTPS, proxy_info.type);
  EXPECT_EQ(proxy_address, proxy_info.address);
}


TEST_F(ProxyDetectTest, DISABLED_TestFirefoxProxyAuto) {
  ProxyInfo proxy_info;
  std::string prefs(kFirefoxHeader);

  prefs.append("user_pref(\"network.proxy.type\", 4);\n");

  EXPECT_TRUE(GetProxyInfo(prefs, &proxy_info));

  EXPECT_EQ(PROXY_NONE, proxy_info.type);
  EXPECT_TRUE(proxy_info.autodetect);
  EXPECT_TRUE(proxy_info.autoconfig_url.empty());
}



TEST_F(ProxyDetectTest, DISABLED_TestFirefoxProxyAutoUrl) {
  ProxyInfo proxy_info;
  std::string prefs(kFirefoxHeader);

  prefs.append(
      "user_pref(\"network.proxy.autoconfig_url\", \"http://a/b.pac\");\n");
  prefs.append("user_pref(\"network.proxy.type\", 2);\n");

  EXPECT_TRUE(GetProxyInfo(prefs, &proxy_info));

  EXPECT_FALSE(proxy_info.autodetect);
  EXPECT_EQ(PROXY_NONE, proxy_info.type);
  EXPECT_EQ(0, proxy_info.autoconfig_url.compare("http://a/b.pac"));
}

}  
