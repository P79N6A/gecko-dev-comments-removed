









#include "webrtc/base/gunit.h"
#include "webrtc/base/stream.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

namespace {
static const int kTimeoutMs = 10000;
}  




class TestStream : public StreamInterface {
 public:
  TestStream() : pos_(0) { }

  virtual StreamState GetState() const { return SS_OPEN; }
  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error) {
    unsigned char* uc_buffer = static_cast<unsigned char*>(buffer);
    for (size_t i = 0; i < buffer_len; ++i) {
      uc_buffer[i] = static_cast<unsigned char>(pos_++);
    }
    if (read)
      *read = buffer_len;
    return SR_SUCCESS;
  }
  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error) {
    if (error)
      *error = -1;
    return SR_ERROR;
  }
  virtual void Close() { }
  virtual bool SetPosition(size_t position) {
    pos_ = position;
    return true;
  }
  virtual bool GetPosition(size_t* position) const {
    if (position) *position = pos_;
    return true;
  }
  virtual bool GetSize(size_t* size) const {
    return false;
  }
  virtual bool GetAvailable(size_t* size) const {
    return false;
  }

 private:
  size_t pos_;
};

bool VerifyTestBuffer(unsigned char* buffer, size_t len,
                      unsigned char value) {
  bool passed = true;
  for (size_t i = 0; i < len; ++i) {
    if (buffer[i] != value++) {
      passed = false;
      break;
    }
  }
  
  memset(buffer, 0, len);
  return passed;
}

void SeekTest(StreamInterface* stream, const unsigned char value) {
  size_t bytes;
  unsigned char buffer[13] = { 0 };
  const size_t kBufSize = sizeof(buffer);

  EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
  EXPECT_EQ(bytes, kBufSize);
  EXPECT_TRUE(VerifyTestBuffer(buffer, kBufSize, value));
  EXPECT_TRUE(stream->GetPosition(&bytes));
  EXPECT_EQ(13U, bytes);

  EXPECT_TRUE(stream->SetPosition(7));

  EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
  EXPECT_EQ(bytes, kBufSize);
  EXPECT_TRUE(VerifyTestBuffer(buffer, kBufSize, value + 7));
  EXPECT_TRUE(stream->GetPosition(&bytes));
  EXPECT_EQ(20U, bytes);
}

TEST(StreamSegment, TranslatesPosition) {
  TestStream* test = new TestStream;
  
  SeekTest(test, 0);
  StreamSegment* segment = new StreamSegment(test);
  
  SeekTest(segment, 20);
  delete segment;
}

TEST(StreamSegment, SupportsArtificialTermination) {
  TestStream* test = new TestStream;

  size_t bytes;
  unsigned char buffer[5000] = { 0 };
  const size_t kBufSize = sizeof(buffer);

  {
    StreamInterface* stream = test;

    
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
    EXPECT_EQ(bytes, kBufSize);
    EXPECT_TRUE(VerifyTestBuffer(buffer, kBufSize, 0));

    
    EXPECT_TRUE(stream->SetPosition(12345));

    
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
    EXPECT_EQ(bytes, kBufSize);
    EXPECT_TRUE(VerifyTestBuffer(buffer, kBufSize, 12345 % 256));
  }

  
  EXPECT_TRUE(test->SetPosition(100));
  StreamSegment* segment = new StreamSegment(test, 500);

  {
    StreamInterface* stream = segment;

    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
    EXPECT_EQ(500U, bytes);
    EXPECT_TRUE(VerifyTestBuffer(buffer, 500, 100));
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_EOS);

    
    EXPECT_FALSE(stream->SetPosition(12345));
    EXPECT_FALSE(stream->SetPosition(501));

    
    EXPECT_TRUE(stream->SetPosition(500));
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_EOS);

    
    EXPECT_TRUE(stream->SetPosition(0));
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_SUCCESS);
    EXPECT_EQ(500U, bytes);
    EXPECT_TRUE(VerifyTestBuffer(buffer, 500, 100));
    EXPECT_EQ(stream->Read(buffer, kBufSize, &bytes, NULL), SR_EOS);
  }

  delete segment;
}

TEST(FifoBufferTest, TestAll) {
  const size_t kSize = 16;
  const char in[kSize * 2 + 1] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
  char out[kSize * 2];
  void* p;
  const void* q;
  size_t bytes;
  FifoBuffer buf(kSize);
  StreamInterface* stream = &buf;

  
  EXPECT_EQ(SS_OPEN, stream->GetState());
  EXPECT_EQ(SR_BLOCK, stream->Read(out, kSize, &bytes, NULL));
  EXPECT_TRUE(NULL != stream->GetReadData(&bytes));
  EXPECT_EQ((size_t)0, bytes);
  stream->ConsumeReadData(0);
  EXPECT_TRUE(NULL != stream->GetWriteBuffer(&bytes));
  EXPECT_EQ(kSize, bytes);
  stream->ConsumeWriteBuffer(0);

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);

  
  EXPECT_EQ(SR_BLOCK, stream->Write(in, kSize, &bytes, NULL));

  
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize));

  
  EXPECT_EQ(SR_BLOCK, stream->Read(out, kSize, &bytes, NULL));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize * 2, &bytes, NULL));
  EXPECT_EQ(bytes, kSize);

  
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize * 2, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));

  
  
  
  
  
  
  
  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize * 3 / 4, &bytes, NULL));
  EXPECT_EQ(kSize * 3 / 4, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 4, &bytes, NULL));
  EXPECT_EQ(kSize / 4 , bytes);
  EXPECT_EQ(0, memcmp(in + kSize / 2, out, kSize / 4));
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2 , bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(kSize / 2 , bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));

  
  stream->GetWriteBuffer(&bytes);
  stream->ConsumeWriteBuffer(0);

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  q = stream->GetReadData(&bytes);
  EXPECT_TRUE(NULL != q);
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(q, in, kSize));
  stream->ConsumeReadData(kSize);
  EXPECT_EQ(SR_BLOCK, stream->Read(out, kSize, &bytes, NULL));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  q = stream->GetReadData(&bytes);
  EXPECT_TRUE(NULL != q);
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(q, in, kSize / 2));
  stream->ConsumeReadData(kSize / 2);
  q = stream->GetReadData(&bytes);
  EXPECT_TRUE(NULL != q);
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(q, in + kSize / 2, kSize / 2));
  stream->ConsumeReadData(kSize / 2);
  EXPECT_EQ(SR_BLOCK, stream->Read(out, kSize, &bytes, NULL));

  
  
  
  
  
  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize * 3 / 4, &bytes, NULL));
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  q = stream->GetReadData(&bytes);
  EXPECT_TRUE(NULL != q);
  EXPECT_EQ(kSize / 4, bytes);
  EXPECT_EQ(0, memcmp(q, in + kSize * 3 / 4, kSize / 4));
  stream->ConsumeReadData(kSize / 4);
  q = stream->GetReadData(&bytes);
  EXPECT_TRUE(NULL != q);
  EXPECT_EQ(kSize / 2, bytes);
  EXPECT_EQ(0, memcmp(q, in, kSize / 2));
  stream->ConsumeReadData(kSize / 2);

  
  stream->GetWriteBuffer(&bytes);
  stream->ConsumeWriteBuffer(0);

  
  p = stream->GetWriteBuffer(&bytes);
  EXPECT_TRUE(NULL != p);
  EXPECT_EQ(kSize, bytes);
  memcpy(p, in, kSize);
  stream->ConsumeWriteBuffer(kSize);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize));

  
  p = stream->GetWriteBuffer(&bytes);
  EXPECT_TRUE(NULL != p);
  EXPECT_EQ(kSize, bytes);
  memcpy(p, in, kSize / 2);
  stream->ConsumeWriteBuffer(kSize / 2);
  p = stream->GetWriteBuffer(&bytes);
  EXPECT_TRUE(NULL != p);
  EXPECT_EQ(kSize / 2, bytes);
  memcpy(p, in + kSize / 2, kSize / 2);
  stream->ConsumeWriteBuffer(kSize / 2);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize));

  
  
  
  
  
  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize * 3 / 4, &bytes, NULL));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  p = stream->GetWriteBuffer(&bytes);
  EXPECT_TRUE(NULL != p);
  EXPECT_EQ(kSize / 4, bytes);
  memcpy(p, in, kSize / 4);
  stream->ConsumeWriteBuffer(kSize / 4);
  p = stream->GetWriteBuffer(&bytes);
  EXPECT_TRUE(NULL != p);
  EXPECT_EQ(kSize / 2, bytes);
  memcpy(p, in + kSize / 4, kSize / 4);
  stream->ConsumeWriteBuffer(kSize / 4);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize * 3 / 4, &bytes, NULL));
  EXPECT_EQ(kSize * 3 / 4, bytes);
  EXPECT_EQ(0, memcmp(in + kSize / 2, out, kSize / 4));
  EXPECT_EQ(0, memcmp(in, out + kSize / 4, kSize / 4));

  
  EXPECT_EQ(SR_BLOCK, stream->Read(out, kSize, &bytes, NULL));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_TRUE(buf.SetCapacity(kSize * 2));
  EXPECT_EQ(SR_SUCCESS, stream->Write(in + kSize, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize * 2, &bytes, NULL));
  EXPECT_EQ(kSize * 2, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize * 2));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_TRUE(buf.SetCapacity(kSize));
  EXPECT_EQ(SR_BLOCK, stream->Write(in, kSize, &bytes, NULL));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize, &bytes, NULL));
  EXPECT_EQ(kSize, bytes);
  EXPECT_EQ(0, memcmp(in, out, kSize));

  
  EXPECT_EQ(SR_SUCCESS, stream->Write(in, kSize / 2, &bytes, NULL));
  stream->Close();
  EXPECT_EQ(SS_CLOSED, stream->GetState());
  EXPECT_EQ(SR_EOS, stream->Write(in, kSize / 2, &bytes, NULL));
  EXPECT_EQ(SR_SUCCESS, stream->Read(out, kSize / 2, &bytes, NULL));
  EXPECT_EQ(0, memcmp(in, out, kSize / 2));
  EXPECT_EQ(SR_EOS, stream->Read(out, kSize / 2, &bytes, NULL));
}

TEST(FifoBufferTest, FullBufferCheck) {
  FifoBuffer buff(10);
  buff.ConsumeWriteBuffer(10);

  size_t free;
  EXPECT_TRUE(buff.GetWriteBuffer(&free) != NULL);
  EXPECT_EQ(0U, free);
}

TEST(FifoBufferTest, WriteOffsetAndReadOffset) {
  const size_t kSize = 16;
  const char in[kSize * 2 + 1] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
  char out[kSize * 2];
  FifoBuffer buf(kSize);

  
  EXPECT_EQ(SR_SUCCESS, buf.Write(in, 14, NULL, NULL));

  
  size_t buffered;
  EXPECT_TRUE(buf.GetBuffered(&buffered));
  EXPECT_EQ(14u, buffered);

  
  buf.ConsumeReadData(10);

  
  size_t remaining;
  EXPECT_TRUE(buf.GetWriteRemaining(&remaining));
  EXPECT_EQ(12u, remaining);

  
  EXPECT_EQ(SR_BLOCK, buf.WriteOffset(in, 10, 12, NULL));

  
  EXPECT_EQ(SR_SUCCESS, buf.WriteOffset(in, 8, 4, NULL));

  
  
  EXPECT_TRUE(buf.GetWriteRemaining(&remaining));
  EXPECT_EQ(12u, remaining);
  buf.ConsumeWriteBuffer(12);

  
  
  size_t read;
  EXPECT_EQ(SR_SUCCESS, buf.ReadOffset(out, 8, 8, &read));
  EXPECT_EQ(8u, read);
  EXPECT_EQ(0, memcmp(out, in, 8));

  
  EXPECT_TRUE(buf.GetBuffered(&buffered));
  EXPECT_EQ(16u, buffered);

  
  EXPECT_EQ(SR_BLOCK, buf.ReadOffset(out, 10, 16, NULL));
}

TEST(AsyncWriteTest, TestWrite) {
  FifoBuffer* buf = new FifoBuffer(100);
  AsyncWriteStream stream(buf, Thread::Current());
  EXPECT_EQ(SS_OPEN, stream.GetState());

  
  
  stream.Write("abc", 3, NULL, NULL);
  char bytes[100];
  size_t count;
  
  
  EXPECT_NE(SR_SUCCESS, buf->ReadOffset(&bytes, 3, 0, &count));
  
  
  EXPECT_TRUE_WAIT(SR_SUCCESS == buf->ReadOffset(&bytes, 3, 0, &count),
                   kTimeoutMs);
  EXPECT_EQ(3u, count);
  EXPECT_EQ(0, memcmp(bytes, "abc", 3));

  
  
  stream.Write("d", 1, &count, NULL);
  stream.Write("e", 1, &count, NULL);
  stream.Write("f", 1, &count, NULL);
  EXPECT_EQ(1u, count);
  
  
  EXPECT_NE(SR_SUCCESS, buf->ReadOffset(&bytes, 3, 3, &count));
  
  
  stream.Flush();
  EXPECT_EQ(SR_SUCCESS, buf->ReadOffset(&bytes, 3, 3, &count));
  EXPECT_EQ(3u, count);
  EXPECT_EQ(0, memcmp(bytes, "def", 3));

  
  
  stream.Write("xyz", 3, &count, NULL);
  EXPECT_EQ(3u, count);
  
  
  EXPECT_NE(SR_SUCCESS, buf->ReadOffset(&bytes, 3, 6, &count));
  
  
  stream.Close();
  EXPECT_EQ(SR_SUCCESS, buf->ReadOffset(&bytes, 3, 6, &count));
  EXPECT_EQ(3u, count);
  EXPECT_EQ(0, memcmp(bytes, "xyz", 3));
  EXPECT_EQ(SS_CLOSED, stream.GetState());

  
  EXPECT_EQ(SR_ERROR, stream.Write("000", 3, NULL, NULL));

}

}  
