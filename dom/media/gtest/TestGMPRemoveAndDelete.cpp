





#include "GMPService.h"
#include "GMPTestMonitor.h"
#include "gmp-api/gmp-video-host.h"
#include "gtest/gtest.h"
#include "mozilla/Services.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIObserverService.h"

#define GMP_DIR_NAME NS_LITERAL_STRING("gmp-fake")
#define GMP_OLD_VERSION NS_LITERAL_STRING("1.0")
#define GMP_NEW_VERSION NS_LITERAL_STRING("1.1")

#define GMP_DELETED_TOPIC "gmp-directory-deleted"

#define EXPECT_OK(X) EXPECT_TRUE(NS_SUCCEEDED(X))

class GMPRemoveTest : public nsIObserver
                    , public GMPVideoDecoderCallbackProxy
{
public:
  GMPRemoveTest();

  NS_DECL_THREADSAFE_ISUPPORTS

  
  
  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                     const char16_t* aData) override;

  
  
  
  void Setup();

  bool CreateVideoDecoder(nsCString aNodeId = EmptyCString());
  void CloseVideoDecoder();

  void DeletePluginDirectory(bool aCanDefer);

  
  GMPErr Decode();

  
  void Wait();

  
  bool IsTerminated();

  
  
  virtual void Decoded(GMPVideoi420Frame* aDecodedFrame) override;
  virtual void Error(GMPErr aError) override;

  
  
  virtual void Terminated() override;

  
  virtual void ReceivedDecodedReferenceFrame(const uint64_t aPictureId) override {}
  virtual void ReceivedDecodedFrame(const uint64_t aPictureId) override {}
  virtual void InputDataExhausted() override {}
  virtual void DrainComplete() override {}
  virtual void ResetComplete() override {}

private:
  virtual ~GMPRemoveTest();

  void gmp_Decode();
  void gmp_GetVideoDecoder(nsCString aNodeId,
                           GMPVideoDecoderProxy** aOutDecoder,
                           GMPVideoHost** aOutHost);
  void GeneratePlugin();

  GMPTestMonitor mTestMonitor;
  nsCOMPtr<nsIThread> mGMPThread;

  bool mIsTerminated;

  
  nsString mTmpPath;
  nsCOMPtr<nsIFile> mTmpDir;

  
  
  nsString mOriginalPath;

  GMPVideoDecoderProxy* mDecoder;
  GMPVideoHost* mHost;
  GMPErr mDecodeResult;
};




TEST(GeckoMediaPlugins, RemoveAndDeleteForcedSimple)
{
  nsRefPtr<GMPRemoveTest> test(new GMPRemoveTest());

  test->Setup();
  test->DeletePluginDirectory(false );
  test->Wait();
}




TEST(GeckoMediaPlugins, RemoveAndDeleteDeferredSimple)
{
  nsRefPtr<GMPRemoveTest> test(new GMPRemoveTest());

  test->Setup();
  test->DeletePluginDirectory(true );
  test->Wait();
}





TEST(GeckoMediaPlugins, RemoveAndDeleteForcedInUse)
{
  nsRefPtr<GMPRemoveTest> test(new GMPRemoveTest());

  test->Setup();
  EXPECT_TRUE(test->CreateVideoDecoder(NS_LITERAL_CSTRING("thisOrigin")));

  
  GMPErr err = test->Decode();
  EXPECT_EQ(err, GMPNoErr);

  test->DeletePluginDirectory(false );
  test->Wait();

  
  EXPECT_FALSE(test->CreateVideoDecoder(NS_LITERAL_CSTRING("thisOrigin")));

  
  EXPECT_TRUE(test->IsTerminated());
}





TEST(GeckoMediaPlugins, RemoveAndDeleteDeferredInUse)
{
  nsRefPtr<GMPRemoveTest> test(new GMPRemoveTest());

  test->Setup();
  EXPECT_TRUE(test->CreateVideoDecoder(NS_LITERAL_CSTRING("thisOrigin")));

  
  GMPErr err = test->Decode();
  EXPECT_EQ(err, GMPNoErr);

  test->DeletePluginDirectory(true );

  
  err = test->Decode();
  EXPECT_EQ(err, GMPNoErr);

  
  EXPECT_FALSE(test->CreateVideoDecoder(NS_LITERAL_CSTRING("otherOrigin")));

  
  EXPECT_TRUE(test->CreateVideoDecoder(NS_LITERAL_CSTRING("thisOrigin")));

  test->CloseVideoDecoder();
  test->Wait();
}

static StaticRefPtr<GeckoMediaPluginService> gService;
static StaticRefPtr<GeckoMediaPluginServiceParent> gServiceParent;

static GeckoMediaPluginService*
GetService()
{
  if (!gService) {
    nsRefPtr<GeckoMediaPluginService> service =
      GeckoMediaPluginService::GetGeckoMediaPluginService();
    gService = service;
  }

  return gService.get();
}

static GeckoMediaPluginServiceParent*
GetServiceParent()
{
  if (!gServiceParent) {
    nsRefPtr<GeckoMediaPluginServiceParent> parent =
      GeckoMediaPluginServiceParent::GetSingleton();
    gServiceParent = parent;
  }

  return gServiceParent.get();
}

NS_IMPL_ISUPPORTS(GMPRemoveTest, nsIObserver)

GMPRemoveTest::GMPRemoveTest()
  : mIsTerminated(false)
  , mDecoder(nullptr)
  , mHost(nullptr)
{
}

GMPRemoveTest::~GMPRemoveTest()
{
  bool exists;
  EXPECT_TRUE(NS_SUCCEEDED(mTmpDir->Exists(&exists)) && !exists);

  EXPECT_OK(GetServiceParent()->AddPluginDirectory(mOriginalPath));
}

void
GMPRemoveTest::Setup()
{
  GeneratePlugin();
  EXPECT_OK(GetServiceParent()->RemovePluginDirectory(mOriginalPath));

  GetServiceParent()->AddPluginDirectory(mTmpPath);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  obs->AddObserver(this, GMP_DELETED_TOPIC, false );

  GetService()->GetThread(getter_AddRefs(mGMPThread));
}

bool
GMPRemoveTest::CreateVideoDecoder(nsCString aNodeId)
{
  GMPVideoHost* host;
  GMPVideoDecoderProxy* decoder = nullptr;

  mGMPThread->Dispatch(
    NS_NewRunnableMethodWithArgs<nsCString, GMPVideoDecoderProxy**, GMPVideoHost**>(
      this, &GMPRemoveTest::gmp_GetVideoDecoder, aNodeId, &decoder, &host),
    NS_DISPATCH_NORMAL);

  mTestMonitor.AwaitFinished();

  if (!decoder) {
    return false;
  }

  GMPVideoCodec codec;
  memset(&codec, 0, sizeof(codec));
  codec.mGMPApiVersion = 33;

  nsTArray<uint8_t> empty;
  mGMPThread->Dispatch(
    NS_NewNonOwningRunnableMethodWithArgs<const GMPVideoCodec&, const nsTArray<uint8_t>&, GMPVideoDecoderCallbackProxy*, int32_t>(
      decoder, &GMPVideoDecoderProxy::InitDecode,
      codec, empty, this, 1 ),
    NS_DISPATCH_SYNC);

  if (mDecoder) {
    CloseVideoDecoder();
  }

  mDecoder = decoder;
  mHost = host;

  return true;
}

void
GMPRemoveTest::gmp_GetVideoDecoder(nsCString aNodeId,
                                   GMPVideoDecoderProxy** aOutDecoder,
                                   GMPVideoHost** aOutHost)
{
  nsTArray<nsCString> tags;
  tags.AppendElement(NS_LITERAL_CSTRING("h264"));

  class Callback : public GetGMPVideoDecoderCallback
  {
  public:
    Callback(GMPTestMonitor* aMonitor, GMPVideoDecoderProxy** aDecoder, GMPVideoHost** aHost)
      : mMonitor(aMonitor), mDecoder(aDecoder), mHost(aHost) { }
    virtual void Done(GMPVideoDecoderProxy* aDecoder, GMPVideoHost* aHost) override {
      *mDecoder = aDecoder;
      *mHost = aHost;
      mMonitor->SetFinished();
    }
  private:
    GMPTestMonitor* mMonitor;
    GMPVideoDecoderProxy** mDecoder;
    GMPVideoHost** mHost;
  };

  UniquePtr<GetGMPVideoDecoderCallback>
    cb(new Callback(&mTestMonitor, aOutDecoder, aOutHost));

  if (NS_FAILED(GetService()->GetGMPVideoDecoder(&tags, aNodeId, Move(cb)))) {
    mTestMonitor.SetFinished();
  }
}

void
GMPRemoveTest::CloseVideoDecoder()
{
  mGMPThread->Dispatch(
    NS_NewNonOwningRunnableMethod(mDecoder, &GMPVideoDecoderProxy::Close),
    NS_DISPATCH_SYNC);

  mDecoder = nullptr;
  mHost = nullptr;
}

void
GMPRemoveTest::DeletePluginDirectory(bool aCanDefer)
{
  GetServiceParent()->RemoveAndDeletePluginDirectory(mTmpPath, aCanDefer);
}

GMPErr
GMPRemoveTest::Decode()
{
  mGMPThread->Dispatch(
    NS_NewNonOwningRunnableMethod(this, &GMPRemoveTest::gmp_Decode),
    NS_DISPATCH_NORMAL);

  mTestMonitor.AwaitFinished();
  return mDecodeResult;
}

void
GMPRemoveTest::gmp_Decode()
{
  
  struct EncodedFrame {
    uint32_t length_;
    uint8_t h264_compat_;
    uint32_t magic_;
    uint32_t width_;
    uint32_t height_;
    uint8_t y_;
    uint8_t u_;
    uint8_t v_;
    uint32_t timestamp_;
  };

  GMPVideoFrame* absFrame;
  GMPErr err = mHost->CreateFrame(kGMPEncodedVideoFrame, &absFrame);
  EXPECT_EQ(err, GMPNoErr);

  GMPUnique<GMPVideoEncodedFrame>::Ptr
    frame(static_cast<GMPVideoEncodedFrame*>(absFrame));
  err = frame->CreateEmptyFrame(sizeof(EncodedFrame) );
  EXPECT_EQ(err, GMPNoErr);

  EncodedFrame* frameData = reinterpret_cast<EncodedFrame*>(frame->Buffer());
  frameData->magic_ = 0x4652414d;
  frameData->width_ = frameData->height_ = 16;

  nsTArray<uint8_t> empty;
  nsresult rv = mDecoder->Decode(Move(frame), false , empty);
  EXPECT_OK(rv);
}

void
GMPRemoveTest::Wait()
{
  mTestMonitor.AwaitFinished();
}

bool
GMPRemoveTest::IsTerminated()
{
  return mIsTerminated;
}


NS_IMETHODIMP
GMPRemoveTest::Observe(nsISupports* aSubject, const char* aTopic,
                       const char16_t* aData)
{
  EXPECT_TRUE(!strcmp(GMP_DELETED_TOPIC, aTopic));

  nsString data(aData);
  if (mTmpPath.Equals(data)) {
    mTestMonitor.SetFinished();
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    obs->RemoveObserver(this, GMP_DELETED_TOPIC);
  }

  return NS_OK;
}


void
GMPRemoveTest::Decoded(GMPVideoi420Frame* aDecodedFrame)
{
  aDecodedFrame->Destroy();
  mDecodeResult = GMPNoErr;
  mTestMonitor.SetFinished();
}


void
GMPRemoveTest::Error(GMPErr aError)
{
  mDecodeResult = aError;
  mTestMonitor.SetFinished();
}


void
GMPRemoveTest::Terminated()
{
  mIsTerminated = true;
}

void
GMPRemoveTest::GeneratePlugin()
{
  nsresult rv;
  nsCOMPtr<nsIFile> gmpDir;
  nsCOMPtr<nsIFile> origDir;
  nsCOMPtr<nsIFile> tmpDir;

  rv = NS_GetSpecialDirectory(NS_GRE_DIR,
                              getter_AddRefs(gmpDir));
  EXPECT_OK(rv);
  rv = gmpDir->Append(GMP_DIR_NAME);
  EXPECT_OK(rv);

  rv = gmpDir->Clone(getter_AddRefs(origDir));
  EXPECT_OK(rv);
  rv = origDir->Append(GMP_OLD_VERSION);
  EXPECT_OK(rv);

  rv = origDir->CopyTo(gmpDir, GMP_NEW_VERSION);
  EXPECT_OK(rv);

  rv = gmpDir->Clone(getter_AddRefs(tmpDir));
  EXPECT_OK(rv);
  rv = tmpDir->Append(GMP_NEW_VERSION);
  EXPECT_OK(rv);

  EXPECT_OK(origDir->GetPath(mOriginalPath));
  EXPECT_OK(tmpDir->GetPath(mTmpPath));
  mTmpDir = tmpDir;
}
