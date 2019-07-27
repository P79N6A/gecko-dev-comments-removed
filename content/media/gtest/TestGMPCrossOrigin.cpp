





#include "gtest/gtest.h"

#include "mozilla/StaticPtr.h"
#include "GMPVideoDecoderProxy.h"
#include "GMPVideoEncoderProxy.h"
#include "GMPService.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsISimpleEnumerator.h"

using namespace mozilla;
using namespace mozilla::gmp;

struct GMPTestRunner
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GMPTestRunner)

  void DoTest(void (GMPTestRunner::*aTestMethod)());
  void RunTestGMPTestCodec();
  void RunTestGMPCrossOrigin();

private:
  ~GMPTestRunner() { }
};

void
GMPTestRunner::RunTestGMPTestCodec()
{
  nsRefPtr<GeckoMediaPluginService> service =
    GeckoMediaPluginService::GetGeckoMediaPluginService();

  GMPVideoHost* host = nullptr;
  GMPVideoDecoderProxy* decoder = nullptr;
  GMPVideoDecoderProxy* decoder2 = nullptr;
  GMPVideoEncoderProxy* encoder = nullptr;

  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("h264"));

  service->GetGMPVideoDecoder(&tags, NS_LITERAL_CSTRING("o"), &host, &decoder2);
  service->GetGMPVideoDecoder(&tags, NS_LITERAL_CSTRING(""), &host, &decoder);

  service->GetGMPVideoEncoder(&tags, NS_LITERAL_CSTRING(""), &host, &encoder);

  EXPECT_TRUE(host);
  EXPECT_TRUE(decoder);
  EXPECT_TRUE(decoder2);
  EXPECT_TRUE(encoder);

  if (decoder) decoder->Close();
  if (decoder2) decoder2->Close();
  if (encoder) encoder->Close();
}

void
GMPTestRunner::RunTestGMPCrossOrigin()
{
  nsRefPtr<GeckoMediaPluginService> service =
    GeckoMediaPluginService::GetGeckoMediaPluginService();

  GMPVideoHost* host = nullptr;
  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("h264"));

  GMPVideoDecoderProxy* decoder1 = nullptr;
  GMPVideoDecoderProxy* decoder2 = nullptr;
  GMPVideoEncoderProxy* encoder1 = nullptr;
  GMPVideoEncoderProxy* encoder2 = nullptr;

  service->GetGMPVideoDecoder(&tags, NS_LITERAL_CSTRING("origin1"), &host, &decoder1);
  service->GetGMPVideoDecoder(&tags, NS_LITERAL_CSTRING("origin2"), &host, &decoder2);
  EXPECT_TRUE(!!decoder1 && !!decoder2 &&
              decoder1->ParentID() != decoder2->ParentID());

  service->GetGMPVideoEncoder(&tags, NS_LITERAL_CSTRING("origin1"), &host, &encoder1);
  service->GetGMPVideoEncoder(&tags, NS_LITERAL_CSTRING("origin2"), &host, &encoder2);
  EXPECT_TRUE(!!encoder1 && !!encoder2 &&
              encoder1->ParentID() != encoder2->ParentID());

  if (decoder2) decoder2->Close();
  if (encoder2) encoder2->Close();

  service->GetGMPVideoDecoder(&tags, NS_LITERAL_CSTRING("origin1"), &host, &decoder2);
  EXPECT_TRUE(!!decoder1 && !!decoder2 &&
              decoder1->ParentID() == decoder2->ParentID());

  service->GetGMPVideoEncoder(&tags, NS_LITERAL_CSTRING("origin1"), &host, &encoder2);
  EXPECT_TRUE(!!encoder1 && !!encoder2 &&
              encoder1->ParentID() == encoder2->ParentID());

  if (decoder1) decoder1->Close();
  if (decoder2) decoder2->Close();
  if (encoder1) encoder1->Close();
  if (encoder2) encoder2->Close();
}

void
GMPTestRunner::DoTest(void (GMPTestRunner::*aTestMethod)())
{
  nsRefPtr<GeckoMediaPluginService> service =
    GeckoMediaPluginService::GetGeckoMediaPluginService();
  nsCOMPtr<nsIThread> thread;

  service->GetThread(getter_AddRefs(thread));
  thread->Dispatch(NS_NewRunnableMethod(this, aTestMethod), NS_DISPATCH_SYNC);
}

TEST(GeckoMediaPlugins, GMPTestCodec) {
  nsRefPtr<GMPTestRunner> runner = new GMPTestRunner();
  runner->DoTest(&GMPTestRunner::RunTestGMPTestCodec);
}

TEST(GeckoMediaPlugins, GMPCrossOrigin) {
  nsRefPtr<GMPTestRunner> runner = new GMPTestRunner();
  runner->DoTest(&GMPTestRunner::RunTestGMPCrossOrigin);
}
