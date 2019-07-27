









#include "webrtc/base/gunit.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/systeminfo.h"

#if defined(CPU_X86) || defined(CPU_ARM)
TEST(SystemInfoTest, CpuVendorNonEmpty) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuVendor: " << info.GetCpuVendor();
  EXPECT_FALSE(info.GetCpuVendor().empty());
}



TEST(SystemInfoTest, CpuVendorIntelAMDARM) {
  rtc::SystemInfo info;
#if defined(CPU_X86)
  EXPECT_TRUE(rtc::string_match(info.GetCpuVendor().c_str(),
                                      "GenuineIntel") ||
              rtc::string_match(info.GetCpuVendor().c_str(),
                                      "AuthenticAMD"));
#elif defined(CPU_ARM)
  EXPECT_TRUE(rtc::string_match(info.GetCpuVendor().c_str(), "ARM"));
#endif
}
#endif  


TEST(SystemInfoTest, GetCpuArchitecture) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuArchitecture: " << info.GetCpuArchitecture();
  rtc::SystemInfo::Architecture architecture = info.GetCpuArchitecture();
#if defined(CPU_X86) || defined(CPU_ARM)
  if (sizeof(intptr_t) == 8) {
    EXPECT_EQ(rtc::SystemInfo::SI_ARCH_X64, architecture);
  } else if (sizeof(intptr_t) == 4) {
#if defined(CPU_ARM)
    EXPECT_EQ(rtc::SystemInfo::SI_ARCH_ARM, architecture);
#else
    EXPECT_EQ(rtc::SystemInfo::SI_ARCH_X86, architecture);
#endif
  }
#endif
}


TEST(SystemInfoTest, CpuCacheSize) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuCacheSize: " << info.GetCpuCacheSize();
  EXPECT_GE(info.GetCpuCacheSize(), 8192);  
  EXPECT_LE(info.GetCpuCacheSize(), 1024 * 1024 * 1024);  
}


TEST(SystemInfoTest, MachineModelKnown) {
  rtc::SystemInfo info;
  EXPECT_FALSE(info.GetMachineModel().empty());
  const char *machine_model = info.GetMachineModel().c_str();
  LOG(LS_INFO) << "MachineModel: " << machine_model;
  bool known = true;
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  
  known = rtc::string_match(machine_model, "MacBookPro*") ||
          rtc::string_match(machine_model, "MacBookAir*") ||
          rtc::string_match(machine_model, "MacBook*") ||
          rtc::string_match(machine_model, "MacPro*") ||
          rtc::string_match(machine_model, "Macmini*") ||
          rtc::string_match(machine_model, "iMac*") ||
          rtc::string_match(machine_model, "Xserve*");
#elif !defined(WEBRTC_IOS)
  
  known = rtc::string_match(info.GetMachineModel().c_str(),
                                  "Not available");
#endif
  if (!known) {
    LOG(LS_WARNING) << "Machine Model Unknown: " << machine_model;
  }
}


TEST(SystemInfoTest, CpuMaxCpuSpeed) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "MaxCpuSpeed: " << info.GetMaxCpuSpeed();
  EXPECT_GT(info.GetMaxCpuSpeed(), 0);
  EXPECT_LT(info.GetMaxCpuSpeed(), 100000);  
}


TEST(SystemInfoTest, CpuCurCpuSpeed) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "MaxCurSpeed: " << info.GetCurCpuSpeed();
  EXPECT_GT(info.GetCurCpuSpeed(), 0);
  EXPECT_LT(info.GetMaxCpuSpeed(), 100000);
}


TEST(SystemInfoTest, MemorySize) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "MemorySize: " << info.GetMemorySize();
  EXPECT_GT(info.GetMemorySize(), -1);
}


TEST(SystemInfoTest, MaxCpus) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "MaxCpus: " << info.GetMaxCpus();
  EXPECT_GT(info.GetMaxCpus(), 0);
}


TEST(SystemInfoTest, MaxPhysicalCpus) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "MaxPhysicalCpus: " << info.GetMaxPhysicalCpus();
  EXPECT_GT(info.GetMaxPhysicalCpus(), 0);
  EXPECT_LE(info.GetMaxPhysicalCpus(), info.GetMaxCpus());
}


TEST(SystemInfoTest, CurCpus) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CurCpus: " << info.GetCurCpus();
  EXPECT_GT(info.GetCurCpus(), 0);
  EXPECT_LE(info.GetCurCpus(), info.GetMaxCpus());
}

#ifdef CPU_X86





TEST(SystemInfoTest, CpuFamily) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuFamily: " << info.GetCpuFamily();
  EXPECT_GT(info.GetCpuFamily(), 0);
}


TEST(SystemInfoTest, CpuModel) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuModel: " << info.GetCpuModel();
  EXPECT_GT(info.GetCpuModel(), 0);
}


TEST(SystemInfoTest, CpuStepping) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuStepping: " << info.GetCpuStepping();
  EXPECT_GT(info.GetCpuStepping(), 0);
}
#else  


TEST(SystemInfoTest, CpuFamily) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuFamily: " << info.GetCpuFamily();
  EXPECT_EQ(0, info.GetCpuFamily());
}


TEST(SystemInfoTest, CpuModel) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuModel: " << info.GetCpuModel();
  EXPECT_EQ(0, info.GetCpuModel());
}


TEST(SystemInfoTest, CpuStepping) {
  rtc::SystemInfo info;
  LOG(LS_INFO) << "CpuStepping: " << info.GetCpuStepping();
  EXPECT_EQ(0, info.GetCpuStepping());
}
#endif  

#if WEBRTC_WIN && !defined(EXCLUDE_D3D9)
TEST(SystemInfoTest, GpuInfo) {
  rtc::SystemInfo info;
  rtc::SystemInfo::GpuInfo gi;
  EXPECT_TRUE(info.GetGpuInfo(&gi));
  LOG(LS_INFO) << "GpuDriver: " << gi.driver;
  EXPECT_FALSE(gi.driver.empty());
  LOG(LS_INFO) << "GpuDriverVersion: " << gi.driver_version;
  EXPECT_FALSE(gi.driver_version.empty());
}
#endif
