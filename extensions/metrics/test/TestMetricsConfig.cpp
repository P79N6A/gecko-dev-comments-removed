







































#include "TestCommon.h"
#include "nsMetricsConfig.h"
#include "nsMetricsService.h"
#include "nsXPCOM.h"
#include "nsILocalFile.h"
#ifndef MOZILLA_1_8_BRANCH
#include "nsIClassInfoImpl.h"
#endif

#include <stdio.h>




NS_DECL_CLASSINFO(nsMetricsService)

static int gTotalTests = 0;
static int gPassedTests = 0;

void TestLoad(const char *testdata_path)
{
  ++gTotalTests;

  nsMetricsConfig config;
  ASSERT_TRUE(config.Init());

  nsCOMPtr<nsILocalFile> dataFile;
  NS_NewNativeLocalFile(nsDependentCString(testdata_path),
                        PR_TRUE, getter_AddRefs(dataFile));
  ASSERT_TRUE(dataFile);

  ASSERT_SUCCESS(dataFile->AppendNative(
                     NS_LITERAL_CSTRING("test_config.xml")));
  ASSERT_SUCCESS(config.Load(dataFile));

  ASSERT_TRUE(config.IsEventEnabled(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                                    NS_LITERAL_STRING("foo")));
  ASSERT_TRUE(config.IsEventEnabled(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                                    NS_LITERAL_STRING("bar")));
  ASSERT_FALSE(config.IsEventEnabled(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                                     NS_LITERAL_STRING("baz")));

  ASSERT_TRUE(config.EventLimit() == 200);
  ASSERT_TRUE(config.UploadInterval() == 1000);
  ASSERT_TRUE(config.HasConfig());
  ++gPassedTests;
}


static PRBool CheckFileContents(nsILocalFile *file, const char *contents)
{
  nsCString nativePath;
  file->GetNativePath(nativePath);

  
  PRFileInfo info;
  ASSERT_TRUE_RET(PR_GetFileInfo(nativePath.get(), &info) == PR_SUCCESS,
                  PR_FALSE);

  char *buf = new char[info.size + 1];
  ASSERT_TRUE_RET(buf, PR_FALSE);

  PRFileDesc *fd = PR_Open(nativePath.get(), PR_RDONLY, 0);
  ASSERT_TRUE_RET(fd, PR_FALSE);

  ASSERT_TRUE_RET(PR_Read(fd, buf, info.size) == info.size, PR_FALSE);
  PR_Close(fd);
  buf[info.size] = '\0';

  
  ASSERT_TRUE_RET(!strcmp(buf, contents), PR_FALSE);
  PR_Delete(nativePath.get());
  delete[] buf;
  return PR_TRUE;
}

void TestSave(const char *temp_data_path)
{
  ++gTotalTests;
  static const char kFilename[] = "test-save.xml";
  static const char kExpectedContents[] =
    "<response xmlns=\"http://www.mozilla.org/metrics\"><config>"
    "<collectors>"
      "<collector type=\"uielement\"/>"
    "</collectors>"
    "<limit events=\"300\"/>"
    "<upload interval=\"500\"/>"
    "</config></response>";

  nsMetricsConfig config;
  ASSERT_TRUE(config.Init());

  
  config.SetEventEnabled(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                         NS_LITERAL_STRING("uielement"), PR_TRUE);
  config.SetUploadInterval(500);
  config.SetEventLimit(300);

  nsCOMPtr<nsILocalFile> outFile;
  NS_NewNativeLocalFile(nsDependentCString(temp_data_path),
                        PR_TRUE, getter_AddRefs(outFile));
  ASSERT_TRUE(outFile);
  ASSERT_SUCCESS(outFile->AppendNative(nsDependentCString(kFilename)));

  ASSERT_SUCCESS(config.Save(outFile));
  ASSERT_TRUE(CheckFileContents(outFile, kExpectedContents));

  
  static const char kExpectedOutputNoEvents[] =
    "<response xmlns=\"http://www.mozilla.org/metrics\"><config>"
    "<collectors/>"
    "<limit events=\"300\"/>"
    "<upload interval=\"500\"/>"
    "</config></response>";

  config.ClearEvents();
  ASSERT_SUCCESS(config.Save(outFile));
  ASSERT_TRUE(CheckFileContents(outFile, kExpectedOutputNoEvents));

  ++gPassedTests;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s test_data_path temp_data_path\n", argv[0]);
    return 1;
  }

  TestLoad(argv[1]);
  TestSave(argv[2]);

  printf("%d/%d tests passed\n", gPassedTests, gTotalTests);
  return 0;
}
