






























#include <elf.h>
#include <stdlib.h>

#include "common/linux/file_id.h"
#include "breakpad_googletest_includes.h"

using namespace google_breakpad;

namespace {
typedef testing::Test FileIDTest;
}

TEST(FileIDTest, FileIDStrip) {
  
  
  
  char exe_name[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_name, PATH_MAX - 1);
  ASSERT_NE(len, -1);
  exe_name[len] = '\0';

  
  char templ[] = "/tmp/file-id-unittest-XXXXXX";
  mktemp(templ);
  char cmdline[4096];
  sprintf(cmdline, "cp \"%s\" \"%s\"", exe_name, templ);
  ASSERT_EQ(system(cmdline), 0);
  sprintf(cmdline, "strip \"%s\"", templ);
  ASSERT_EQ(system(cmdline), 0);

  uint8_t identifier1[sizeof(MDGUID)];
  uint8_t identifier2[sizeof(MDGUID)];
  FileID fileid1(exe_name);
  EXPECT_TRUE(fileid1.ElfFileIdentifier(identifier1));
  FileID fileid2(templ);
  EXPECT_TRUE(fileid2.ElfFileIdentifier(identifier2));
  char identifier_string1[37];
  char identifier_string2[37];
  FileID::ConvertIdentifierToString(identifier1, identifier_string1,
                                    37);
  FileID::ConvertIdentifierToString(identifier2, identifier_string2,
                                    37);
  EXPECT_STREQ(identifier_string1, identifier_string2);
  unlink(templ);
}

struct ElfClass32 {
  typedef Elf32_Ehdr Ehdr;
  typedef Elf32_Shdr Shdr;
  static const int kClass = ELFCLASS32;
};

struct ElfClass64 {
  typedef Elf64_Ehdr Ehdr;
  typedef Elf64_Shdr Shdr;
  static const int kClass = ELFCLASS64;
};

template<typename ElfClass>
struct ElfishElf {
  static const size_t kTextSectionSize = 128;
  typedef typename ElfClass::Ehdr Ehdr;
  typedef typename ElfClass::Shdr Shdr;

  Ehdr elf_header;
  Shdr text_header;
  Shdr string_header;
  char text_section[kTextSectionSize];
  char string_section[8];

  static void Populate(ElfishElf* elf) {
    memset(elf, 0, sizeof(ElfishElf));
    memcpy(elf, ELFMAG, SELFMAG);
    elf->elf_header.e_ident[EI_CLASS] = ElfClass::kClass;
    elf->elf_header.e_shoff = offsetof(ElfishElf, text_header);
    elf->elf_header.e_shnum = 2;
    elf->elf_header.e_shstrndx = 1;
    elf->text_header.sh_name = 0;
    elf->text_header.sh_type = SHT_PROGBITS;
    elf->text_header.sh_offset = offsetof(ElfishElf, text_section);
    elf->text_header.sh_size = kTextSectionSize;
    for (size_t i = 0; i < kTextSectionSize; ++i) {
      elf->text_section[i] = i * 3;
    }
    elf->string_header.sh_offset = offsetof(ElfishElf, string_section);
    strcpy(elf->string_section, ".text");
  }
};

TEST(FileIDTest, ElfClass) {
  uint8_t identifier[sizeof(MDGUID)];
  const char expected_identifier_string[] =
      "80808080-8080-0000-0000-008080808080";
  char identifier_string[sizeof(expected_identifier_string)];

  ElfishElf<ElfClass32> elf32;
  ElfishElf<ElfClass32>::Populate(&elf32);
  EXPECT_TRUE(FileID::ElfFileIdentifierFromMappedFile(&elf32, identifier));
  FileID::ConvertIdentifierToString(identifier, identifier_string,
                                    sizeof(identifier_string));
  EXPECT_STREQ(expected_identifier_string, identifier_string);

  memset(identifier, 0, sizeof(identifier));
  memset(identifier_string, 0, sizeof(identifier_string));

  ElfishElf<ElfClass64> elf64;
  ElfishElf<ElfClass64>::Populate(&elf64);
  EXPECT_TRUE(FileID::ElfFileIdentifierFromMappedFile(&elf64, identifier));
  FileID::ConvertIdentifierToString(identifier, identifier_string,
                                    sizeof(identifier_string));
  EXPECT_STREQ(expected_identifier_string, identifier_string);
}
