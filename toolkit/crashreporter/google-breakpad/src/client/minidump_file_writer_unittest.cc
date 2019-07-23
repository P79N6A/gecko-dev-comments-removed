






































#include <fcntl.h>
#include <unistd.h>

#include "minidump_file_writer-inl.h"

using google_breakpad::MinidumpFileWriter;

#define ASSERT_TRUE(cond) \
if (!(cond)) { \
  fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    return false; \
}

#define ASSERT_EQ(e1, e2) ASSERT_TRUE((e1) == (e2))
#define ASSERT_NE(e1, e2) ASSERT_TRUE((e1) != (e2))

struct StringStructure {
  unsigned long integer_value;
  MDLocationDescriptor first_string;
  MDLocationDescriptor second_string;
};

struct ArrayStructure {
  unsigned char char_value;
  unsigned short short_value;
  unsigned long long_value;
};

typedef struct {
  unsigned long count;
  ArrayStructure array[0];
} ObjectAndArrayStructure;

static bool WriteFile(const char *path) {
  MinidumpFileWriter writer;
  if (writer.Open(path)) {
    
    google_breakpad::TypedMDRVA<StringStructure> strings(&writer);
    ASSERT_TRUE(strings.Allocate());
    strings.get()->integer_value = 0xBEEF;
    const char *first = "First String";
    ASSERT_TRUE(writer.WriteString(first, 0, &strings.get()->first_string));
    const wchar_t *second = L"Second String";
    ASSERT_TRUE(writer.WriteString(second, 0, &strings.get()->second_string));

    
    google_breakpad::TypedMDRVA<ArrayStructure> array(&writer);
    unsigned int count = 10;
    ASSERT_TRUE(array.AllocateArray(count));
    for (unsigned int i = 0; i < count; ++i) {
      ArrayStructure local;
      local.char_value = i;
      local.short_value = i + 1;
      local.long_value = i + 2;
      ASSERT_TRUE(array.CopyIndex(i, &local));
    }

    
    google_breakpad::TypedMDRVA<ObjectAndArrayStructure> obj_array(&writer);
    ASSERT_TRUE(obj_array.AllocateObjectAndArray(count,
                                                 sizeof(ArrayStructure)));
    obj_array.get()->count = count;
    for (unsigned int i = 0; i < count; ++i) {
      ArrayStructure local;
      local.char_value = i;
      local.short_value = i + 1;
      local.long_value = i + 2;
      ASSERT_TRUE(obj_array.CopyIndexAfterObject(i, &local, sizeof(local)));
    }
  }

  return writer.Close();
}

static bool CompareFile(const char *path) {
  unsigned long expected[] = {
#if defined(__BIG_ENDIAN__)
    0x0000beef, 0x0000001e, 0x00000018, 0x00000020, 0x00000038, 0x00000000, 
    0x00000018, 0x00460069, 0x00720073, 0x00740020, 0x00530074, 0x00720069,
    0x006e0067, 0x00000000, 0x0000001a, 0x00530065, 0x0063006f, 0x006e0064,
    0x00200053, 0x00740072, 0x0069006e, 0x00670000, 0x00000001, 0x00000002,
    0x01000002, 0x00000003, 0x02000003, 0x00000004, 0x03000004, 0x00000005,
    0x04000005, 0x00000006, 0x05000006, 0x00000007, 0x06000007, 0x00000008,
    0x07000008, 0x00000009, 0x08000009, 0x0000000a, 0x0900000a, 0x0000000b,
    0x0000000a, 0x00000001, 0x00000002, 0x01000002, 0x00000003, 0x02000003,
    0x00000004, 0x03000004, 0x00000005, 0x04000005, 0x00000006, 0x05000006,
    0x00000007, 0x06000007, 0x00000008, 0x07000008, 0x00000009, 0x08000009,
    0x0000000a, 0x0900000a, 0x0000000b, 0x00000000
#else
    0x0000beef, 0x0000001e, 0x00000018, 0x00000020,
    0x00000038, 0x00000000, 0x00000018, 0x00690046,
    0x00730072, 0x00200074, 0x00740053, 0x00690072,
    0x0067006e, 0x00000000, 0x0000001a, 0x00650053,
    0x006f0063, 0x0064006e, 0x00530020, 0x00720074,
    0x006e0069, 0x00000067, 0x00011e00, 0x00000002,
    0x00021e01, 0x00000003, 0x00031e02, 0x00000004,
    0x00041e03, 0x00000005, 0x00051e04, 0x00000006,
    0x00061e05, 0x00000007, 0x00071e06, 0x00000008,
    0x00081e07, 0x00000009, 0x00091e08, 0x0000000a,
    0x000a1e09, 0x0000000b, 0x0000000a, 0x00011c00,
    0x00000002, 0x00021c01, 0x00000003, 0x00031c02,
    0x00000004, 0x00041c03, 0x00000005, 0x00051c04,
    0x00000006, 0x00061c05, 0x00000007, 0x00071c06,
    0x00000008, 0x00081c07, 0x00000009, 0x00091c08,
    0x0000000a, 0x000a1c09, 0x0000000b, 0x00000000,
#endif
  };
  unsigned int expected_byte_count = sizeof(expected);
  int fd = open(path, O_RDONLY, 0600);
  void *buffer = malloc(expected_byte_count);
  ASSERT_NE(fd, -1);
  ASSERT_TRUE(buffer);
  ASSERT_EQ(read(fd, buffer, expected_byte_count), expected_byte_count);

  char *b1, *b2;
  b1 = (char*)buffer;
  b2 = (char*)expected;
  while (*b1 == *b2) {
    b1++;
    b2++;
  }

  printf("%d\n",b1 - (char*)buffer);

  ASSERT_EQ(memcmp(buffer, expected, expected_byte_count), 0);
  return true;
}

static bool RunTests() {
  const char *path = "/tmp/minidump_file_writer_unittest.dmp";
  ASSERT_TRUE(WriteFile(path));
  ASSERT_TRUE(CompareFile(path));
  unlink(path);
  return true;
}

extern "C" int main(int argc, const char *argv[]) {
  return RunTests() ? 0 : 1;
}
