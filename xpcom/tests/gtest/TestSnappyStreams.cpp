





#include <algorithm>
#include "gtest/gtest.h"
#include "mozilla/SnappyCompressOutputStream.h"
#include "mozilla/SnappyUncompressInputStream.h"
#include "nsIPipe.h"
#include "nsStreamUtils.h"
#include "nsString.h"
#include "nsStringStream.h"
#include "nsTArray.h"

namespace {

using mozilla::SnappyCompressOutputStream;
using mozilla::SnappyUncompressInputStream;

static void CreateData(uint32_t aNumBytes, nsTArray<char>& aDataOut)
{
  static const char data[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec egestas "
    "purus eu condimentum iaculis. In accumsan leo eget odio porttitor, non "
    "rhoncus nulla vestibulum. Etiam lacinia consectetur nisl nec "
    "sollicitudin. Sed fringilla accumsan diam, pulvinar varius massa. Duis "
    "mollis dignissim felis, eget tempus nisi tristique ut. Fusce euismod, "
    "lectus non lacinia tempor, tellus diam suscipit quam, eget hendrerit "
    "lacus nunc fringilla ante. Sed ultrices massa vitae risus molestie, ut "
    "finibus quam laoreet nullam.";
  static const uint32_t dataLength = sizeof(data) - 1;

  aDataOut.SetCapacity(aNumBytes);

  while (aNumBytes > 0) {
    uint32_t amount = std::min(dataLength, aNumBytes);
    aDataOut.AppendElements(data, amount);
    aNumBytes -= amount;
  }
}

static void
WriteAllAndClose(nsIOutputStream* aStream, const nsTArray<char>& aData)
{
  uint32_t offset = 0;
  uint32_t remaining = aData.Length();
  while (remaining > 0) {
    uint32_t numWritten;
    nsresult rv = aStream->Write(aData.Elements() + offset, remaining,
                                 &numWritten);
    ASSERT_TRUE(NS_SUCCEEDED(rv));
    if (numWritten < 1) {
      break;
    }
    offset += numWritten;
    remaining -= numWritten;
  }
  aStream->Close();
}

static already_AddRefed<nsIOutputStream>
CompressPipe(nsIInputStream** aReaderOut)
{
  nsCOMPtr<nsIOutputStream> pipeWriter;

  nsresult rv = NS_NewPipe(aReaderOut, getter_AddRefs(pipeWriter));
  if (NS_FAILED(rv)) { return nullptr; }

  nsCOMPtr<nsIOutputStream> compress =
    new SnappyCompressOutputStream(pipeWriter);
  return compress.forget();
}


static void TestCompress(uint32_t aNumBytes)
{
  
  
  ASSERT_GT(aNumBytes, 1024u);

  nsCOMPtr<nsIInputStream> pipeReader;
  nsCOMPtr<nsIOutputStream> compress = CompressPipe(getter_AddRefs(pipeReader));
  ASSERT_TRUE(compress);

  nsTArray<char> inputData;
  CreateData(aNumBytes, inputData);

  WriteAllAndClose(compress, inputData);

  nsAutoCString outputData;
  nsresult rv = NS_ConsumeStream(pipeReader, UINT32_MAX, outputData);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  ASSERT_LT(outputData.Length(), inputData.Length());
}



static void TestCompressUncompress(uint32_t aNumBytes)
{
  nsCOMPtr<nsIInputStream> pipeReader;
  nsCOMPtr<nsIOutputStream> compress = CompressPipe(getter_AddRefs(pipeReader));
  ASSERT_TRUE(compress);

  nsCOMPtr<nsIInputStream> uncompress =
    new SnappyUncompressInputStream(pipeReader);

  nsTArray<char> inputData;
  CreateData(aNumBytes, inputData);

  WriteAllAndClose(compress, inputData);

  nsAutoCString outputData;
  nsresult rv = NS_ConsumeStream(uncompress, UINT32_MAX, outputData);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  ASSERT_EQ(inputData.Length(), outputData.Length());
  for (uint32_t i = 0; i < inputData.Length(); ++i) {
    EXPECT_EQ(inputData[i], outputData.get()[i]) << "Byte " << i;
  }
}

static void TestUncompressCorrupt(const char* aCorruptData,
                                  uint32_t aCorruptLength)
{
  nsCOMPtr<nsIInputStream> source;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(source), aCorruptData,
                                      aCorruptLength);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIInputStream> uncompress =
    new SnappyUncompressInputStream(source);

  nsAutoCString outputData;
  rv = NS_ConsumeStream(uncompress, UINT32_MAX, outputData);
  ASSERT_EQ(NS_ERROR_CORRUPTED_CONTENT, rv);
}

} 

TEST(SnappyStream, Compress_32k)
{
  TestCompress(32 * 1024);
}

TEST(SnappyStream, Compress_64k)
{
  TestCompress(64 * 1024);
}

TEST(SnappyStream, Compress_128k)
{
  TestCompress(128 * 1024);
}

TEST(SnappyStream, CompressUncompress_0)
{
  TestCompressUncompress(0);
}

TEST(SnappyStream, CompressUncompress_1)
{
  TestCompressUncompress(1);
}

TEST(SnappyStream, CompressUncompress_32)
{
  TestCompressUncompress(32);
}

TEST(SnappyStream, CompressUncompress_1k)
{
  TestCompressUncompress(1024);
}

TEST(SnappyStream, CompressUncompress_32k)
{
  TestCompressUncompress(32 * 1024);
}

TEST(SnappyStream, CompressUncompress_64k)
{
  TestCompressUncompress(64 * 1024);
}

TEST(SnappyStream, CompressUncompress_128k)
{
  TestCompressUncompress(128 * 1024);
}




TEST(SnappyStream, CompressUncompress_256k_less_13)
{
  TestCompressUncompress((256 * 1024) - 13);
}

TEST(SnappyStream, CompressUncompress_256k)
{
  TestCompressUncompress(256 * 1024);
}

TEST(SnappyStream, CompressUncompress_256k_plus_13)
{
  TestCompressUncompress((256 * 1024) + 13);
}

TEST(SnappyStream, UncompressCorruptStreamIdentifier)
{
  static const char data[] = "This is not a valid compressed stream";
  TestUncompressCorrupt(data, strlen(data));
}

TEST(SnappyStream, UncompressCorruptCompressedDataLength)
{
  static const char data[] = "\xff\x06\x00\x00sNaPpY" 
                             "\x00\x99\x00\x00This is not a valid compressed stream";
  static const uint32_t dataLength = (sizeof(data) / sizeof(const char)) - 1;
  TestUncompressCorrupt(data, dataLength);
}

TEST(SnappyStream, UncompressCorruptCompressedDataContent)
{
  static const char data[] = "\xff\x06\x00\x00sNaPpY" 
                             "\x00\x25\x00\x00This is not a valid compressed stream";
  static const uint32_t dataLength = (sizeof(data) / sizeof(const char)) - 1;
  TestUncompressCorrupt(data, dataLength);
}
