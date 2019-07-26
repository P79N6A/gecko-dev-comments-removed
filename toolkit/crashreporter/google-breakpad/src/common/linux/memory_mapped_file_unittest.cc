































#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/memory_mapped_file.h"
#include "common/tests/auto_tempdir.h"
#include "common/tests/file_utils.h"
#include "common/using_std_string.h"

using google_breakpad::AutoTempDir;
using google_breakpad::MemoryMappedFile;
using google_breakpad::WriteFile;

namespace {

class MemoryMappedFileTest : public testing::Test {
 protected:
  void ExpectNoMappedData(const MemoryMappedFile& mapped_file) {
    EXPECT_TRUE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() == NULL);
    EXPECT_EQ(0U, mapped_file.size());
  }
};

}  

TEST_F(MemoryMappedFileTest, DefaultConstructor) {
  MemoryMappedFile mapped_file;
  ExpectNoMappedData(mapped_file);
}

TEST_F(MemoryMappedFileTest, UnmapWithoutMap) {
  MemoryMappedFile mapped_file;
  mapped_file.Unmap();
}

TEST_F(MemoryMappedFileTest, MapNonexistentFile) {
  {
    MemoryMappedFile mapped_file("nonexistent-file");
    ExpectNoMappedData(mapped_file);
  }
  {
    MemoryMappedFile mapped_file;
    EXPECT_FALSE(mapped_file.Map("nonexistent-file"));
    ExpectNoMappedData(mapped_file);
  }
}

TEST_F(MemoryMappedFileTest, MapEmptyFile) {
  AutoTempDir temp_dir;
  string test_file = temp_dir.path() + "/empty_file";
  ASSERT_TRUE(WriteFile(test_file.c_str(), NULL, 0));

  {
    MemoryMappedFile mapped_file(test_file.c_str());
    ExpectNoMappedData(mapped_file);
  }
  {
    MemoryMappedFile mapped_file;
    EXPECT_TRUE(mapped_file.Map(test_file.c_str()));
    ExpectNoMappedData(mapped_file);
  }
}

TEST_F(MemoryMappedFileTest, MapNonEmptyFile) {
  char data[256];
  size_t data_size = sizeof(data);
  for (size_t i = 0; i < data_size; ++i) {
    data[i] = i;
  }

  AutoTempDir temp_dir;
  string test_file = temp_dir.path() + "/test_file";
  ASSERT_TRUE(WriteFile(test_file.c_str(), data, data_size));

  {
    MemoryMappedFile mapped_file(test_file.c_str());
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data, mapped_file.data(), data_size));
  }
  {
    MemoryMappedFile mapped_file;
    EXPECT_TRUE(mapped_file.Map(test_file.c_str()));
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data, mapped_file.data(), data_size));
  }
}

TEST_F(MemoryMappedFileTest, RemapAfterMap) {
  char data1[256];
  size_t data1_size = sizeof(data1);
  for (size_t i = 0; i < data1_size; ++i) {
    data1[i] = i;
  }

  char data2[50];
  size_t data2_size = sizeof(data2);
  for (size_t i = 0; i < data2_size; ++i) {
    data2[i] = 255 - i;
  }

  AutoTempDir temp_dir;
  string test_file1 = temp_dir.path() + "/test_file1";
  string test_file2 = temp_dir.path() + "/test_file2";
  ASSERT_TRUE(WriteFile(test_file1.c_str(), data1, data1_size));
  ASSERT_TRUE(WriteFile(test_file2.c_str(), data2, data2_size));

  {
    MemoryMappedFile mapped_file(test_file1.c_str());
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data1_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data1, mapped_file.data(), data1_size));

    mapped_file.Map(test_file2.c_str());
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data2_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data2, mapped_file.data(), data2_size));
  }
  {
    MemoryMappedFile mapped_file;
    EXPECT_TRUE(mapped_file.Map(test_file1.c_str()));
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data1_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data1, mapped_file.data(), data1_size));

    mapped_file.Map(test_file2.c_str());
    EXPECT_FALSE(mapped_file.content().IsEmpty());
    EXPECT_TRUE(mapped_file.data() != NULL);
    EXPECT_EQ(data2_size, mapped_file.size());
    EXPECT_EQ(0, memcmp(data2, mapped_file.data(), data2_size));
  }
}
