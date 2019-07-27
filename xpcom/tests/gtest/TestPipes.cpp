





#include <algorithm>
#include "gtest/gtest.h"
#include "Helpers.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsICloneableInputStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIPipe.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsStreamUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "prprf.h"
#include "prinrval.h"

using namespace mozilla;

#define ITERATIONS      33333
char kTestPattern[] = "My hovercraft is full of eels.\n";

bool gTrace = false;

static nsresult
WriteAll(nsIOutputStream *os, const char *buf, uint32_t bufLen, uint32_t *lenWritten)
{
    const char *p = buf;
    *lenWritten = 0;
    while (bufLen) {
        uint32_t n;
        nsresult rv = os->Write(p, bufLen, &n);
        if (NS_FAILED(rv)) return rv;
        p += n;
        bufLen -= n;
        *lenWritten += n;
    }
    return NS_OK;
}

class nsReceiver final : public nsIRunnable {
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_IMETHOD Run() override {
        nsresult rv;
        char buf[101];
        uint32_t count;
        PRIntervalTime start = PR_IntervalNow();
        while (true) {
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {

                break;
            }

            if (gTrace) {
                buf[count] = '\0';
                printf("read: %s\n", buf);
            }
            mCount += count;
        }
        PRIntervalTime end = PR_IntervalNow();
        printf("read  %d bytes, time = %dms\n", mCount,
               PR_IntervalToMilliseconds(end - start));
        return rv;
    }

    explicit nsReceiver(nsIInputStream* in) : mIn(in), mCount(0) {
    }

    uint32_t GetBytesRead() { return mCount; }

private:
    ~nsReceiver() {}

protected:
    nsCOMPtr<nsIInputStream> mIn;
    uint32_t            mCount;
};

NS_IMPL_ISUPPORTS(nsReceiver, nsIRunnable)

nsresult
TestPipe(nsIInputStream* in, nsIOutputStream* out)
{
    nsRefPtr<nsReceiver> receiver = new nsReceiver(in);
    if (!receiver)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;

    nsCOMPtr<nsIThread> thread;
    rv = NS_NewThread(getter_AddRefs(thread), receiver);
    if (NS_FAILED(rv)) return rv;

    uint32_t total = 0;
    PRIntervalTime start = PR_IntervalNow();
    for (uint32_t i = 0; i < ITERATIONS; i++) {
        uint32_t writeCount;
        char *buf = PR_smprintf("%d %s", i, kTestPattern);
        uint32_t len = strlen(buf);
        rv = WriteAll(out, buf, len, &writeCount);
        if (gTrace) {
            printf("wrote: ");
            for (uint32_t j = 0; j < writeCount; j++) {
                putc(buf[j], stdout);
            }
            printf("\n");
        }
        PR_smprintf_free(buf);
        if (NS_FAILED(rv)) return rv;
        total += writeCount;
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    PRIntervalTime end = PR_IntervalNow();

    thread->Shutdown();

    printf("wrote %d bytes, time = %dms\n", total,
           PR_IntervalToMilliseconds(end - start));
    EXPECT_EQ(receiver->GetBytesRead(), total);

    return NS_OK;
}



class nsShortReader final : public nsIRunnable {
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_IMETHOD Run() override {
        nsresult rv;
        char buf[101];
        uint32_t count;
        uint32_t total = 0;
        while (true) {
            
            
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {
                break;
            }

            if (gTrace) {
                
                buf[count] = '\0';

                printf("read %d bytes: %s\n", count, buf);
            }

            Received(count);
            total += count;
        }
        printf("read  %d bytes\n", total);
        return rv;
    }

    explicit nsShortReader(nsIInputStream* in) : mIn(in), mReceived(0) {
        mMon = new ReentrantMonitor("nsShortReader");
    }

    void Received(uint32_t count) {
        ReentrantMonitorAutoEnter mon(*mMon);
        mReceived += count;
        mon.Notify();
    }

    uint32_t WaitForReceipt(const uint32_t aWriteCount) {
        ReentrantMonitorAutoEnter mon(*mMon);
        uint32_t result = mReceived;

        while (result < aWriteCount) {
            mon.Wait();

            EXPECT_TRUE(mReceived > result);
            result = mReceived;
        }

        mReceived = 0;
        return result;
    }

private:
    ~nsShortReader() {}

protected:
    nsCOMPtr<nsIInputStream> mIn;
    uint32_t                 mReceived;
    ReentrantMonitor*        mMon;
};

NS_IMPL_ISUPPORTS(nsShortReader, nsIRunnable)

nsresult
TestShortWrites(nsIInputStream* in, nsIOutputStream* out)
{
    nsRefPtr<nsShortReader> receiver = new nsShortReader(in);
    if (!receiver)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;

    nsCOMPtr<nsIThread> thread;
    rv = NS_NewThread(getter_AddRefs(thread), receiver);
    if (NS_FAILED(rv)) return rv;

    uint32_t total = 0;
    for (uint32_t i = 0; i < ITERATIONS; i++) {
        uint32_t writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        uint32_t len = strlen(buf);
        len = len * rand() / RAND_MAX;
        len = std::min(1u, len);
        rv = WriteAll(out, buf, len, &writeCount);
        if (NS_FAILED(rv)) return rv;
        EXPECT_EQ(writeCount, len);
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);
        PR_smprintf_free(buf);
        
        out->Flush();
        

#ifdef DEBUG
        const uint32_t received =
          receiver->WaitForReceipt(writeCount);
        EXPECT_EQ(received, writeCount);
#endif
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    thread->Shutdown();

    printf("wrote %d bytes\n", total);

    return NS_OK;
}



class nsPump final : public nsIRunnable
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_IMETHOD Run() override {
        nsresult rv;
        uint32_t count;
        while (true) {
            rv = mOut->WriteFrom(mIn, ~0U, &count);
            if (NS_FAILED(rv)) {
                printf("Write failed\n");
                break;
            }
            if (count == 0) {
                printf("EOF count = %d\n", mCount);
                break;
            }

            if (gTrace) {
                printf("Wrote: %d\n", count);
            }
            mCount += count;
        }
        mOut->Close();
        return rv;
    }

    nsPump(nsIInputStream* in,
           nsIOutputStream* out)
        : mIn(in), mOut(out), mCount(0) {
    }

private:
    ~nsPump() {}

protected:
    nsCOMPtr<nsIInputStream>      mIn;
    nsCOMPtr<nsIOutputStream>     mOut;
    uint32_t                            mCount;
};

NS_IMPL_ISUPPORTS(nsPump, nsIRunnable)

TEST(Pipes, ChainedPipes)
{
    nsresult rv;
    if (gTrace) {
        printf("TestChainedPipes\n");
    }

    nsCOMPtr<nsIInputStream> in1;
    nsCOMPtr<nsIOutputStream> out1;
    rv = NS_NewPipe(getter_AddRefs(in1), getter_AddRefs(out1), 20, 1999);
    if (NS_FAILED(rv)) return;

    nsCOMPtr<nsIInputStream> in2;
    nsCOMPtr<nsIOutputStream> out2;
    rv = NS_NewPipe(getter_AddRefs(in2), getter_AddRefs(out2), 200, 401);
    if (NS_FAILED(rv)) return;

    nsRefPtr<nsPump> pump = new nsPump(in1, out2);
    if (pump == nullptr) return;

    nsCOMPtr<nsIThread> thread;
    rv = NS_NewThread(getter_AddRefs(thread), pump);
    if (NS_FAILED(rv)) return;

    nsRefPtr<nsReceiver> receiver = new nsReceiver(in2);
    if (receiver == nullptr) return;

    nsCOMPtr<nsIThread> receiverThread;
    rv = NS_NewThread(getter_AddRefs(receiverThread), receiver);
    if (NS_FAILED(rv)) return;

    uint32_t total = 0;
    for (uint32_t i = 0; i < ITERATIONS; i++) {
        uint32_t writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        uint32_t len = strlen(buf);
        len = len * rand() / RAND_MAX;
        len = std::max(1u, len);
        rv = WriteAll(out1, buf, len, &writeCount);
        if (NS_FAILED(rv)) return;
        EXPECT_EQ(writeCount, len);
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);

        PR_smprintf_free(buf);
    }
    if (gTrace) {
        printf("wrote total of %d bytes\n", total);
    }
    rv = out1->Close();
    if (NS_FAILED(rv)) return;

    thread->Shutdown();
    receiverThread->Shutdown();
}



void
RunTests(uint32_t segSize, uint32_t segCount)
{
    nsresult rv;
    nsCOMPtr<nsIInputStream> in;
    nsCOMPtr<nsIOutputStream> out;
    uint32_t bufSize = segSize * segCount;
    if (gTrace) {
        printf("Testing New Pipes: segment size %d buffer size %d\n", segSize, bufSize);
        printf("Testing long writes...\n");
    }
    rv = NS_NewPipe(getter_AddRefs(in), getter_AddRefs(out), segSize, bufSize);
    EXPECT_TRUE(NS_SUCCEEDED(rv));
    rv = TestPipe(in, out);
    EXPECT_TRUE(NS_SUCCEEDED(rv));

    if (gTrace) {
        printf("Testing short writes...\n");
    }
    rv = NS_NewPipe(getter_AddRefs(in), getter_AddRefs(out), segSize, bufSize);
    EXPECT_TRUE(NS_SUCCEEDED(rv));
    rv = TestShortWrites(in, out);
    EXPECT_TRUE(NS_SUCCEEDED(rv));
}

TEST(Pipes, Main)
{
    RunTests(16, 1);
    RunTests(4096, 16);
}



namespace {

static const uint32_t DEFAULT_SEGMENT_SIZE = 4 * 1024;



static void TestPipe2(uint32_t aNumBytes,
                      uint32_t aSegmentSize = DEFAULT_SEGMENT_SIZE)
{
  nsCOMPtr<nsIInputStream> reader;
  nsCOMPtr<nsIOutputStream> writer;

  uint32_t maxSize = std::max(aNumBytes, aSegmentSize);

  nsresult rv = NS_NewPipe(getter_AddRefs(reader), getter_AddRefs(writer),
                           aSegmentSize, maxSize);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> inputData;
  testing::CreateData(aNumBytes, inputData);
  testing::WriteAllAndClose(writer, inputData);
  testing::ConsumeAndValidateStream(reader, inputData);
}

} 

TEST(Pipes, Blocking_32k)
{
  TestPipe2(32 * 1024);
}

TEST(Pipes, Blocking_64k)
{
  TestPipe2(64 * 1024);
}

TEST(Pipes, Blocking_128k)
{
  TestPipe2(128 * 1024);
}



namespace {


















static void TestPipeClone(uint32_t aTotalBytes,
                          uint32_t aNumWrites,
                          uint32_t aNumInitialClones,
                          uint32_t aNumToCloseAfterWrite,
                          uint32_t aNumToCloneAfterWrite,
                          uint32_t aNumStreamsToReadPerWrite,
                          uint32_t aSegmentSize = DEFAULT_SEGMENT_SIZE)
{
  nsCOMPtr<nsIInputStream> reader;
  nsCOMPtr<nsIOutputStream> writer;

  uint32_t maxSize = std::max(aTotalBytes, aSegmentSize);

  
  
  nsresult rv = NS_NewPipe(getter_AddRefs(reader), getter_AddRefs(writer),
                           aSegmentSize, maxSize,
                           true, false); 
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsICloneableInputStream> cloneable = do_QueryInterface(reader);
  ASSERT_TRUE(cloneable);
  ASSERT_TRUE(cloneable->GetCloneable());

  nsTArray<nsCString> outputDataList;

  nsTArray<nsCOMPtr<nsIInputStream>> streamList;

  
  streamList.AppendElement(reader);
  outputDataList.AppendElement();

  
  
  for (uint32_t i = 0; i < aNumInitialClones; ++i) {
    nsCOMPtr<nsIInputStream>* clone = streamList.AppendElement();
    rv = cloneable->Clone(getter_AddRefs(*clone));
    ASSERT_TRUE(NS_SUCCEEDED(rv));
    ASSERT_TRUE(*clone);

    outputDataList.AppendElement();
  }

  nsTArray<char> inputData;
  testing::CreateData(aTotalBytes, inputData);

  const uint32_t bytesPerWrite = ((aTotalBytes - 1)/ aNumWrites) + 1;
  uint32_t offset = 0;
  uint32_t remaining = aTotalBytes;
  uint32_t nextStreamToRead = 0;

  while (remaining) {
    uint32_t numToWrite = std::min(bytesPerWrite, remaining);
    testing::Write(writer, inputData, offset, numToWrite);
    offset += numToWrite;
    remaining -= numToWrite;

    
    
    for (uint32_t i = 0; i < aNumToCloseAfterWrite &&
                         streamList.Length() > 1; ++i) {

      uint32_t lastIndex = streamList.Length() - 1;
      streamList[lastIndex]->Close();
      streamList.RemoveElementAt(lastIndex);
      outputDataList.RemoveElementAt(lastIndex);

      if (nextStreamToRead >= streamList.Length()) {
        nextStreamToRead = 0;
      }
    }

    
    
    
    for (uint32_t i = 0; i < aNumToCloneAfterWrite; ++i) {
      nsCOMPtr<nsIInputStream>* clone = streamList.AppendElement();
      rv = cloneable->Clone(getter_AddRefs(*clone));
      ASSERT_TRUE(NS_SUCCEEDED(rv));
      ASSERT_TRUE(*clone);

      
      
      nsCString* outputData = outputDataList.AppendElement();
      *outputData = outputDataList[0];
    }

    
    
    
    for (uint32_t i = 0; i < aNumStreamsToReadPerWrite; ++i) {
      nsCOMPtr<nsIInputStream>& stream = streamList[nextStreamToRead];
      nsCString& outputData = outputDataList[nextStreamToRead];

      
      
      
      nsAutoCString tmpOutputData;
      rv = NS_ConsumeStream(stream, UINT32_MAX, tmpOutputData);
      ASSERT_TRUE(rv == NS_BASE_STREAM_WOULD_BLOCK || NS_SUCCEEDED(rv));
      ASSERT_GE(tmpOutputData.Length(), numToWrite);

      outputData += tmpOutputData;

      nextStreamToRead += 1;
      if (nextStreamToRead >= streamList.Length()) {
        
        
        
        

        nextStreamToRead = 0;
      }
    }
  }

  rv = writer->Close();
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsDependentCSubstring inputString(inputData.Elements(), inputData.Length());

  
  
  
  for (uint32_t i = 0; i < streamList.Length(); ++i) {
    nsCOMPtr<nsIInputStream>& stream = streamList[i];
    nsCString& outputData = outputDataList[i];

    nsAutoCString tmpOutputData;
    rv = NS_ConsumeStream(stream, UINT32_MAX, tmpOutputData);
    ASSERT_TRUE(rv == NS_BASE_STREAM_WOULD_BLOCK || NS_SUCCEEDED(rv));
    stream->Close();

    
    outputData += tmpOutputData;

    ASSERT_EQ(inputString.Length(), outputData.Length());
    ASSERT_TRUE(inputString.Equals(outputData));
  }
}

} 

TEST(Pipes, Clone_BeforeWrite_ReadAtEnd)
{
  TestPipeClone(32 * 1024, 
                16,        
                3,         
                0,         
                0,         
                0);        
}

TEST(Pipes, Clone_BeforeWrite_ReadDuringWrite)
{
  
  
  

  TestPipeClone(32 * 1024, 
                16,        
                3,         
                0,         
                0,         
                4);        
}

TEST(Pipes, Clone_DuringWrite_ReadAtEnd)
{
  TestPipeClone(32 * 1024, 
                16,        
                0,         
                0,         
                1,         
                0);        
}

TEST(Pipes, Clone_DuringWrite_ReadDuringWrite)
{
  TestPipeClone(32 * 1024, 
                16,        
                0,         
                0,         
                1,         
                1);        
}

TEST(Pipes, Clone_DuringWrite_ReadDuringWrite_CloseDuringWrite)
{
  
  
  

  TestPipeClone(32 * 1024, 
                16,        
                1,         
                1,         
                2,         
                3);        
}

TEST(Pipes, Write_AsyncWait)
{
  nsCOMPtr<nsIAsyncInputStream> reader;
  nsCOMPtr<nsIAsyncOutputStream> writer;

  const uint32_t segmentSize = 1024;
  const uint32_t numSegments = 1;

  nsresult rv = NS_NewPipe2(getter_AddRefs(reader), getter_AddRefs(writer),
                            true, true,  
                            segmentSize, numSegments);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> inputData;
  testing::CreateData(segmentSize, inputData);

  uint32_t numWritten = 0;
  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_EQ(NS_BASE_STREAM_WOULD_BLOCK, rv);

  nsRefPtr<testing::OutputStreamCallback> cb =
    new testing::OutputStreamCallback();

  rv = writer->AsyncWait(cb, 0, 0, nullptr);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  ASSERT_FALSE(cb->Called());

  testing::ConsumeAndValidateStream(reader, inputData);

  ASSERT_TRUE(cb->Called());
}

TEST(Pipes, Write_AsyncWait_Clone)
{
  nsCOMPtr<nsIAsyncInputStream> reader;
  nsCOMPtr<nsIAsyncOutputStream> writer;

  const uint32_t segmentSize = 1024;
  const uint32_t numSegments = 1;

  nsresult rv = NS_NewPipe2(getter_AddRefs(reader), getter_AddRefs(writer),
                            true, true,  
                            segmentSize, numSegments);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIInputStream> clone;
  rv = NS_CloneInputStream(reader, getter_AddRefs(clone));
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> inputData;
  testing::CreateData(segmentSize, inputData);

  uint32_t numWritten = 0;
  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_EQ(NS_BASE_STREAM_WOULD_BLOCK, rv);

  nsRefPtr<testing::OutputStreamCallback> cb =
    new testing::OutputStreamCallback();

  rv = writer->AsyncWait(cb, 0, 0, nullptr);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  ASSERT_FALSE(cb->Called());

  testing::ConsumeAndValidateStream(reader, inputData);

  ASSERT_FALSE(cb->Called());

  testing::ConsumeAndValidateStream(clone, inputData);

  ASSERT_TRUE(cb->Called());
}

TEST(Pipes, Write_AsyncWait_Clone_CloseOriginal)
{
  nsCOMPtr<nsIAsyncInputStream> reader;
  nsCOMPtr<nsIAsyncOutputStream> writer;

  const uint32_t segmentSize = 1024;
  const uint32_t numSegments = 1;

  nsresult rv = NS_NewPipe2(getter_AddRefs(reader), getter_AddRefs(writer),
                            true, true,  
                            segmentSize, numSegments);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIInputStream> clone;
  rv = NS_CloneInputStream(reader, getter_AddRefs(clone));
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> inputData;
  testing::CreateData(segmentSize, inputData);

  uint32_t numWritten = 0;
  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_EQ(NS_BASE_STREAM_WOULD_BLOCK, rv);

  nsRefPtr<testing::OutputStreamCallback> cb =
    new testing::OutputStreamCallback();

  rv = writer->AsyncWait(cb, 0, 0, nullptr);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  ASSERT_FALSE(cb->Called());

  testing::ConsumeAndValidateStream(clone, inputData);

  ASSERT_FALSE(cb->Called());

  reader->Close();

  ASSERT_TRUE(cb->Called());
}

namespace {

NS_METHOD
CloseDuringReadFunc(nsIInputStream *aReader,
                    void* aClosure,
                    const char* aFromSegment,
                    uint32_t aToOffset,
                    uint32_t aCount,
                    uint32_t* aWriteCountOut)
{
  MOZ_ASSERT(aReader);
  MOZ_ASSERT(aClosure);
  MOZ_ASSERT(aFromSegment);
  MOZ_ASSERT(aWriteCountOut);
  MOZ_ASSERT(aToOffset == 0);

  
  
  
  
  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aReader->Close()));

  nsTArray<char>* buffer = static_cast<nsTArray<char>*>(aClosure);
  buffer->AppendElements(aFromSegment, aCount);

  *aWriteCountOut = aCount;

  return NS_OK;
}

void
TestCloseDuringRead(uint32_t aSegmentSize, uint32_t aDataSize)
{
  nsCOMPtr<nsIInputStream> reader;
  nsCOMPtr<nsIOutputStream> writer;

  const uint32_t maxSize = aSegmentSize;

  nsresult rv = NS_NewPipe(getter_AddRefs(reader), getter_AddRefs(writer),
                           aSegmentSize, maxSize);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> inputData;

  testing::CreateData(aDataSize, inputData);

  uint32_t numWritten = 0;
  rv = writer->Write(inputData.Elements(), inputData.Length(), &numWritten);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsTArray<char> outputData;

  uint32_t numRead = 0;
  rv = reader->ReadSegments(CloseDuringReadFunc, &outputData,
                            inputData.Length(), &numRead);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_EQ(inputData.Length(), numRead);

  ASSERT_EQ(inputData, outputData);

  uint64_t available;
  rv = reader->Available(&available);
  ASSERT_EQ(NS_BASE_STREAM_CLOSED, rv);
}

} 

TEST(Pipes, Close_During_Read_Partial_Segment)
{
  TestCloseDuringRead(1024, 512);
}

TEST(Pipes, Close_During_Read_Full_Segment)
{
  TestCloseDuringRead(1024, 1024);
}
