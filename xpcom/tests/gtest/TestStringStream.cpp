





#include "gtest/gtest.h"
#include "Helpers.h"
#include "nsICloneableInputStream.h"
#include "nsStringStream.h"

namespace {

static void TestStringStream(uint32_t aNumBytes)
{
  nsTArray<char> inputData;
  testing::CreateData(aNumBytes, inputData);
  nsDependentCSubstring inputString(inputData.Elements(), inputData.Length());

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewCStringInputStream(getter_AddRefs(stream), inputString);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  testing::ConsumeAndValidateStream(stream, inputString);
}

static void TestStringStreamClone(uint32_t aNumBytes)
{
  nsTArray<char> inputData;
  testing::CreateData(aNumBytes, inputData);
  nsDependentCSubstring inputString(inputData.Elements(), inputData.Length());

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewCStringInputStream(getter_AddRefs(stream), inputString);
  ASSERT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsICloneableInputStream> cloneable = do_QueryInterface(stream);
  ASSERT_TRUE(cloneable != nullptr);
  ASSERT_TRUE(cloneable->GetCloneable());

  nsCOMPtr<nsIInputStream> clone;
  rv = cloneable->Clone(getter_AddRefs(clone));

  testing::ConsumeAndValidateStream(stream, inputString);

  
  stream = nullptr;

  testing::ConsumeAndValidateStream(clone, inputString);
}

} 

TEST(StringStream, Simple_4k)
{
  TestStringStream(1024 * 4);
}

TEST(StringStream, Clone_4k)
{
  TestStringStreamClone(1024 * 4);
}
