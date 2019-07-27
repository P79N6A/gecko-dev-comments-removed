





#include <stdlib.h>
#include "gtest/gtest.h"
#include "Helpers.h"
#include "nsCOMPtr.h"
#include "nsICloneableInputStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIStorageStream.h"

namespace {
void
WriteData(nsIOutputStream* aOut, nsTArray<char>& aData, uint32_t aNumBytes,
          nsACString& aDataWritten)
{
  uint32_t n;
  nsresult rv = aOut->Write(aData.Elements(), aNumBytes, &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  aDataWritten.Append(aData.Elements(), aNumBytes);
}

} 
TEST(StorageStreams, Main)
{
  
  nsTArray<char> kData;
  testing::CreateData(4096, kData);

  
  nsAutoCString dataWritten;

  nsresult rv;
  nsCOMPtr<nsIStorageStream> stor;

  rv = NS_NewStorageStream(kData.Length(), UINT32_MAX, getter_AddRefs(stor));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIOutputStream> out;
  rv = stor->GetOutputStream(0, getter_AddRefs(out));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  WriteData(out, kData, kData.Length(), dataWritten);
  WriteData(out, kData, kData.Length(), dataWritten);

  rv = out->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  out = nullptr;

  nsCOMPtr<nsIInputStream> in;
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsICloneableInputStream> cloneable = do_QueryInterface(in);
  ASSERT_TRUE(cloneable != nullptr);
  ASSERT_TRUE(cloneable->GetCloneable());

  nsCOMPtr<nsIInputStream> clone;
  rv = cloneable->Clone(getter_AddRefs(clone));

  testing::ConsumeAndValidateStream(in, dataWritten);
  testing::ConsumeAndValidateStream(clone, dataWritten);
  in = nullptr;
  clone = nullptr;

  
  

  rv = stor->GetOutputStream(dataWritten.Length(), getter_AddRefs(out));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  WriteData(out, kData, kData.Length(), dataWritten);
  WriteData(out, kData, kData.Length(), dataWritten);
  WriteData(out, kData, kData.Length(), dataWritten);
  WriteData(out, kData, 11, dataWritten);

  rv = out->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  out = nullptr;

  
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  testing::ConsumeAndValidateStream(in, dataWritten);
  in = nullptr;
}
