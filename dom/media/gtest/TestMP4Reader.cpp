




#include "gtest/gtest.h"
#include "MP4Reader.h"
#include "MP4Decoder.h"
#include "SharedThreadPool.h"
#include "MockMediaResource.h"
#include "MockMediaDecoderOwner.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/TimeRanges.h"

using namespace mozilla;
using namespace mozilla::dom;

class TestBinding
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TestBinding);

  nsRefPtr<MP4Decoder> decoder;
  nsRefPtr<MockMediaResource> resource;
  nsRefPtr<MP4Reader> reader;

  explicit TestBinding(const char* aFileName = "gizmo.mp4")
    : decoder(new MP4Decoder())
    , resource(new MockMediaResource(aFileName))
    , reader(new MP4Reader(decoder))
  {
    EXPECT_EQ(NS_OK, Preferences::SetBool(
                       "media.fragmented-mp4.use-blank-decoder", true));

    EXPECT_EQ(NS_OK, resource->Open(nullptr));
    decoder->SetResource(resource);

    reader->Init(nullptr);
    reader->EnsureTaskQueue();
    {
      
      
      ReentrantMonitorAutoEnter mon(decoder->GetReentrantMonitor());
      reader->SetStartTime(0);
    }
  }

  void Init() {
    nsCOMPtr<nsIThread> thread;
    nsresult rv = NS_NewThread(getter_AddRefs(thread),
                               NS_NewRunnableMethod(this, &TestBinding::ReadMetadata));
    EXPECT_EQ(NS_OK, rv);
    thread->Shutdown();
  }

private:
  virtual ~TestBinding()
  {
    {
      nsRefPtr<MediaTaskQueue> queue = reader->GetTaskQueue();
      nsCOMPtr<nsIRunnable> task = NS_NewRunnableMethod(reader, &MP4Reader::Shutdown);
      
      
      queue->Dispatch(task.forget(), AbstractThread::AssertDispatchSuccess,
                      AbstractThread::TailDispatch);
      queue->AwaitShutdownAndIdle();
    }
    decoder = nullptr;
    resource = nullptr;
    reader = nullptr;
    SharedThreadPool::SpinUntilShutdown();
  }

  void ReadMetadata()
  {
    MediaInfo info;
    MetadataTags* tags;
    EXPECT_EQ(NS_OK, reader->ReadMetadata(&info, &tags));
  }
};

TEST(MP4Reader, BufferedRange)
{
  nsRefPtr<TestBinding> b = new TestBinding();
  b->Init();

  
  b->resource->MockAddBufferedRange(248400, 327455);

  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  EXPECT_EQ(NS_OK, b->reader->GetBuffered(ranges));
  EXPECT_EQ(1U, ranges->Length());
  double start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(0, &start));
  EXPECT_NEAR(270000 / 90000.0, start, 0.000001);
  double end = 0;
  EXPECT_EQ(NS_OK, ranges->End(0, &end));
  EXPECT_NEAR(360000 / 90000.0, end, 0.000001);
}

TEST(MP4Reader, BufferedRangeMissingLastByte)
{
  nsRefPtr<TestBinding> b = new TestBinding();
  b->Init();

  
  b->resource->MockClearBufferedRanges();
  b->resource->MockAddBufferedRange(248400, 324912);
  b->resource->MockAddBufferedRange(324913, 327455);

  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  EXPECT_EQ(NS_OK, b->reader->GetBuffered(ranges));
  EXPECT_EQ(1U, ranges->Length());
  double start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(0, &start));
  EXPECT_NEAR(270000.0 / 90000.0, start, 0.000001);
  double end = 0;
  EXPECT_EQ(NS_OK, ranges->End(0, &end));
  EXPECT_NEAR(357000 / 90000.0, end, 0.000001);
}

TEST(MP4Reader, BufferedRangeSyncFrame)
{
  nsRefPtr<TestBinding> b = new TestBinding();
  b->Init();

  
  
  b->resource->MockClearBufferedRanges();
  b->resource->MockAddBufferedRange(146336, 327455);

  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  EXPECT_EQ(NS_OK, b->reader->GetBuffered(ranges));
  EXPECT_EQ(1U, ranges->Length());
  double start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(0, &start));
  EXPECT_NEAR(270000.0 / 90000.0, start, 0.000001);
  double end = 0;
  EXPECT_EQ(NS_OK, ranges->End(0, &end));
  EXPECT_NEAR(360000 / 90000.0, end, 0.000001);
}

TEST(MP4Reader, CompositionOrder)
{
  nsRefPtr<TestBinding> b = new TestBinding("mediasource_test.mp4");
  b->Init();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  b->resource->MockClearBufferedRanges();
  
  b->resource->MockAddBufferedRange(48, 5503);   
  b->resource->MockAddBufferedRange(5503, 5648); 
  b->resource->MockAddBufferedRange(6228, 6803); 

  
  b->resource->MockAddBufferedRange(5648, 6228);
  b->resource->MockAddBufferedRange(6803, 7383);
  b->resource->MockAddBufferedRange(7618, 8199);
  b->resource->MockAddBufferedRange(8199, 8779);
  b->resource->MockAddBufferedRange(8962, 9563);
  b->resource->MockAddBufferedRange(9734, 10314);
  b->resource->MockAddBufferedRange(10314, 10895);
  b->resource->MockAddBufferedRange(11207, 11787);
  b->resource->MockAddBufferedRange(12035, 12616);
  b->resource->MockAddBufferedRange(12616, 13196);
  b->resource->MockAddBufferedRange(13220, 13901);

  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  EXPECT_EQ(NS_OK, b->reader->GetBuffered(ranges));
  EXPECT_EQ(2U, ranges->Length());

  double start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(0, &start));
  EXPECT_NEAR(166.0 / 2500.0, start, 0.000001);
  double end = 0;
  EXPECT_EQ(NS_OK, ranges->End(0, &end));
  EXPECT_NEAR(332.0 / 2500.0, end, 0.000001);

  start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(1, &start));
  EXPECT_NEAR(581.0 / 2500.0, start, 0.000001);
  end = 0;
  EXPECT_EQ(NS_OK, ranges->End(1, &end));
  EXPECT_NEAR(11255.0 / 44100.0, end, 0.000001);
}

TEST(MP4Reader, Normalised)
{
  nsRefPtr<TestBinding> b = new TestBinding("mediasource_test.mp4");
  b->Init();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  b->resource->MockClearBufferedRanges();
  b->resource->MockAddBufferedRange(48, 13901);

  nsRefPtr<TimeRanges> ranges = new TimeRanges();
  EXPECT_EQ(NS_OK, b->reader->GetBuffered(ranges));
  EXPECT_EQ(1U, ranges->Length());

  double start = 0;
  EXPECT_EQ(NS_OK, ranges->Start(0, &start));
  EXPECT_NEAR(166.0 / 2500.0, start, 0.000001);
  double end = 0;
  EXPECT_EQ(NS_OK, ranges->End(0, &end));
  EXPECT_NEAR(11255.0 / 44100.0, end, 0.000001);
}
