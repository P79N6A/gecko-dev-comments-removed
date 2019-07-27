





#include <stdlib.h>
#include "nsString.h"
#include "gtest/gtest.h"

TEST(Encoding, GoodSurrogatePair)
{
  
  
  const char16_t goodPairData[] = {  0xD800, 0xDF02, 0x65, 0x78, 0x0 };
  nsDependentString goodPair16(goodPairData);

  uint32_t byteCount = 0;
  char* goodPair8 = ToNewUTF8String(goodPair16, &byteCount);
  EXPECT_TRUE(!!goodPair8);

  EXPECT_EQ(byteCount, 6u);

  const unsigned char expected8[] =
    { 0xF0, 0x90, 0x8C, 0x82, 0x65, 0x78, 0x0 };
  EXPECT_EQ(0, memcmp(expected8, goodPair8, sizeof(expected8)));

  
  
  nsDependentCString expected((const char*)expected8);
  EXPECT_EQ(0, CompareUTF8toUTF16(expected, goodPair16));

  free(goodPair8);
}

TEST(Encoding, BackwardsSurrogatePair)
{
  
  
  const char16_t backwardsPairData[] = { 0xDDDD, 0xD863, 0x65, 0x78, 0x0 };
  nsDependentString backwardsPair16(backwardsPairData);

  uint32_t byteCount = 0;
  char* backwardsPair8 = ToNewUTF8String(backwardsPair16, &byteCount);
  EXPECT_TRUE(!!backwardsPair8);

  EXPECT_EQ(byteCount, 8u);

  const unsigned char expected8[] =
    { 0xEF, 0xBF, 0xBD, 0xEF, 0xBF, 0xBD, 0x65, 0x78, 0x0 };
  EXPECT_EQ(0, memcmp(expected8, backwardsPair8, sizeof(expected8)));

  
  
  nsDependentCString expected((const char*)expected8);
  EXPECT_EQ(0, CompareUTF8toUTF16(expected, backwardsPair16));

  free(backwardsPair8);
}

TEST(Encoding, MalformedUTF16OrphanHighSurrogate)
{
  
  
  const char16_t highSurrogateData[] = { 0xD863, 0x74, 0x65, 0x78, 0x74, 0x0 };
  nsDependentString highSurrogate16(highSurrogateData);

  uint32_t byteCount = 0;
  char* highSurrogate8 = ToNewUTF8String(highSurrogate16, &byteCount);
  EXPECT_TRUE(!!highSurrogate8);

  EXPECT_EQ(byteCount, 7u);

  const unsigned char expected8[] =
    { 0xEF, 0xBF, 0xBD, 0x74, 0x65, 0x78, 0x74, 0x0 };
  EXPECT_EQ(0, memcmp(expected8, highSurrogate8, sizeof(expected8)));

  
  
  nsDependentCString expected((const char*)expected8);
  EXPECT_EQ(0, CompareUTF8toUTF16(expected, highSurrogate16));

  free(highSurrogate8);
}

TEST(Encoding, MalformedUTF16OrphanLowSurrogate)
{
  
  
  const char16_t lowSurrogateData[] = { 0xDDDD, 0x74, 0x65, 0x78, 0x74, 0x0 };
  nsDependentString lowSurrogate16(lowSurrogateData);

  uint32_t byteCount = 0;
  char* lowSurrogate8 = ToNewUTF8String(lowSurrogate16, &byteCount);
  EXPECT_TRUE(!!lowSurrogate8);

  EXPECT_EQ(byteCount, 7u);

  const unsigned char expected8[] =
    { 0xEF, 0xBF, 0xBD, 0x74, 0x65, 0x78, 0x74, 0x0 };
  EXPECT_EQ(0, memcmp(expected8, lowSurrogate8, sizeof(expected8)));

  
  
  nsDependentCString expected((const char*)expected8);
  EXPECT_EQ(0, CompareUTF8toUTF16(expected, lowSurrogate16));

  free(lowSurrogate8);
}
