







#include "Helpers.h"

#include <algorithm>
#include "gtest/gtest.h"
#include "nsIOutputStream.h"
#include "nsStreamUtils.h"
#include "nsTArray.h"

namespace testing {



void
CreateData(uint32_t aNumBytes, nsTArray<char>& aDataOut)
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



void
Write(nsIOutputStream* aStream, const nsTArray<char>& aData, uint32_t aOffset,
      uint32_t aNumBytes)
{
  uint32_t remaining =
    std::min(aNumBytes, static_cast<uint32_t>(aData.Length() - aOffset));

  while (remaining > 0) {
    uint32_t numWritten;
    nsresult rv = aStream->Write(aData.Elements() + aOffset, remaining,
                                 &numWritten);
    ASSERT_TRUE(NS_SUCCEEDED(rv));
    if (numWritten < 1) {
      break;
    }
    aOffset += numWritten;
    remaining -= numWritten;
  }
}


void
WriteAllAndClose(nsIOutputStream* aStream, const nsTArray<char>& aData)
{
  Write(aStream, aData, 0, aData.Length());
  aStream->Close();
}



void
ConsumeAndValidateStream(nsIInputStream* aStream,
                         const nsTArray<char>& aExpectedData)
{
  nsDependentCSubstring data(aExpectedData.Elements(), aExpectedData.Length());
  ConsumeAndValidateStream(aStream, data);
}



void
ConsumeAndValidateStream(nsIInputStream* aStream,
                         const nsACString& aExpectedData)
{
  nsAutoCString outputData;
  nsresult rv = NS_ConsumeStream(aStream, UINT32_MAX, outputData);
  ASSERT_TRUE(NS_SUCCEEDED(rv));
  ASSERT_EQ(aExpectedData.Length(), outputData.Length());
  ASSERT_TRUE(aExpectedData.Equals(outputData));
}

} 
