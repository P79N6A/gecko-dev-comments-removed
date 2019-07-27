



#include <stdlib.h>
#include "nsIStorageStream.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"
#include "gtest/gtest.h"

TEST(TestStorageStreams, Main)
{
  char kData[4096];
  memset(kData, 0, sizeof(kData));

  nsresult rv;
  nsCOMPtr<nsIStorageStream> stor;

  rv = NS_NewStorageStream(4096, UINT32_MAX, getter_AddRefs(stor));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  nsCOMPtr<nsIOutputStream> out;
  rv = stor->GetOutputStream(0, getter_AddRefs(out));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  uint32_t n;

  rv = out->Write(kData, sizeof(kData), &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Write(kData, sizeof(kData), &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  out = nullptr;

  nsCOMPtr<nsIInputStream> in;
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  char buf[4096];

  
  do {
    rv = in->Read(buf, sizeof(buf), &n);
    EXPECT_TRUE(NS_SUCCEEDED(rv));
  } while (n != 0);

  rv = in->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  in = nullptr;

  
  

  rv = stor->GetOutputStream(8192, getter_AddRefs(out));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Write(kData, sizeof(kData), &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Write(kData, sizeof(kData), &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Write(kData, sizeof(kData), &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Write(kData, 11, &n);
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  rv = out->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  out = nullptr;

  
  rv = stor->NewInputStream(0, getter_AddRefs(in));
  EXPECT_TRUE(NS_SUCCEEDED(rv));

  
  do {
    rv = in->Read(buf, sizeof(buf), &n);
    EXPECT_TRUE(NS_SUCCEEDED(rv));
  } while (n != 0);

  rv = in->Close();
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  in = nullptr;
}
