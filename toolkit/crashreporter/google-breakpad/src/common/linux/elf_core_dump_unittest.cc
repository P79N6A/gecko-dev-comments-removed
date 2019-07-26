






























#include <sys/procfs.h>

#include <set>
#include <string>

#include "breakpad_googletest_includes.h"
#include "common/linux/elf_core_dump.h"
#include "common/linux/memory_mapped_file.h"
#include "common/tests/file_utils.h"
#include "common/linux/tests/crash_generator.h"
#include "common/using_std_string.h"

using google_breakpad::AutoTempDir;
using google_breakpad::CrashGenerator;
using google_breakpad::ElfCoreDump;
using google_breakpad::MemoryMappedFile;
using google_breakpad::MemoryRange;
using google_breakpad::WriteFile;
using std::set;

TEST(ElfCoreDumpTest, DefaultConstructor) {
  ElfCoreDump core;
  EXPECT_FALSE(core.IsValid());
  EXPECT_EQ(NULL, core.GetHeader());
  EXPECT_EQ(0U, core.GetProgramHeaderCount());
  EXPECT_EQ(NULL, core.GetProgramHeader(0));
  EXPECT_EQ(NULL, core.GetFirstProgramHeaderOfType(PT_LOAD));
  EXPECT_FALSE(core.GetFirstNote().IsValid());
}

TEST(ElfCoreDumpTest, TestElfHeader) {
  ElfCoreDump::Ehdr header;
  memset(&header, 0, sizeof(header));

  AutoTempDir temp_dir;
  string core_path = temp_dir.path() + "/core";
  const char* core_file = core_path.c_str();
  MemoryMappedFile mapped_core_file;
  ElfCoreDump core;

  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header) - 1));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());
  EXPECT_EQ(NULL, core.GetHeader());
  EXPECT_EQ(0U, core.GetProgramHeaderCount());
  EXPECT_EQ(NULL, core.GetProgramHeader(0));
  EXPECT_EQ(NULL, core.GetFirstProgramHeaderOfType(PT_LOAD));
  EXPECT_FALSE(core.GetFirstNote().IsValid());

  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_ident[0] = ELFMAG0;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_ident[1] = ELFMAG1;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_ident[2] = ELFMAG2;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_ident[3] = ELFMAG3;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_ident[4] = ElfCoreDump::kClass;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_version = EV_CURRENT;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_FALSE(core.IsValid());

  header.e_type = ET_CORE;
  ASSERT_TRUE(WriteFile(core_file, &header, sizeof(header)));
  ASSERT_TRUE(mapped_core_file.Map(core_file));
  core.SetContent(mapped_core_file.content());
  EXPECT_TRUE(core.IsValid());
}

TEST(ElfCoreDumpTest, ValidCoreFile) {
  CrashGenerator crash_generator;
  if (!crash_generator.HasDefaultCorePattern()) {
    fprintf(stderr, "ElfCoreDumpTest.ValidCoreFile test is skipped "
            "due to non-default core pattern");
    return;
  }

  const unsigned kNumOfThreads = 3;
  const unsigned kCrashThread = 1;
  const int kCrashSignal = SIGABRT;
  
  
  if (!crash_generator.CreateChildCrash(kNumOfThreads, kCrashThread,
                                        kCrashSignal, NULL)) {
    fprintf(stderr, "ElfCoreDumpTest.ValidCoreFile test is skipped "
            "due to no core dump generated");
    return;
  }
  pid_t expected_crash_thread_id = crash_generator.GetThreadId(kCrashThread);
  set<pid_t> expected_thread_ids;
  for (unsigned i = 0; i < kNumOfThreads; ++i) {
    expected_thread_ids.insert(crash_generator.GetThreadId(i));
  }

  MemoryMappedFile mapped_core_file;
  ASSERT_TRUE(mapped_core_file.Map(crash_generator.GetCoreFilePath().c_str()));

  ElfCoreDump core;
  core.SetContent(mapped_core_file.content());
  EXPECT_TRUE(core.IsValid());

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  size_t num_nt_prpsinfo = 0;
  size_t num_nt_prstatus = 0;
#if defined(__i386__) || defined(__x86_64__)
  size_t num_nt_fpregset = 0;
#endif
#if defined(__i386__)
  size_t num_nt_prxfpreg = 0;
#endif
  set<pid_t> actual_thread_ids;
  ElfCoreDump::Note note = core.GetFirstNote();
  while (note.IsValid()) {
    MemoryRange name = note.GetName();
    MemoryRange description = note.GetDescription();
    EXPECT_FALSE(name.IsEmpty());
    EXPECT_FALSE(description.IsEmpty());

    switch (note.GetType()) {
      case NT_PRPSINFO: {
        EXPECT_TRUE(description.data() != NULL);
        EXPECT_EQ(sizeof(elf_prpsinfo), description.length());
        ++num_nt_prpsinfo;
        break;
      }
      case NT_PRSTATUS: {
        EXPECT_TRUE(description.data() != NULL);
        EXPECT_EQ(sizeof(elf_prstatus), description.length());
        const elf_prstatus* status = description.GetData<elf_prstatus>(0);
        actual_thread_ids.insert(status->pr_pid);
        if (num_nt_prstatus == 0) {
          EXPECT_EQ(expected_crash_thread_id, status->pr_pid);
          EXPECT_EQ(kCrashSignal, status->pr_info.si_signo);
        }
        ++num_nt_prstatus;
        break;
      }
#if defined(__i386__) || defined(__x86_64__)
      case NT_FPREGSET: {
        EXPECT_TRUE(description.data() != NULL);
        EXPECT_EQ(sizeof(user_fpregs_struct), description.length());
        ++num_nt_fpregset;
        break;
      }
#endif
#if defined(__i386__)
      case NT_PRXFPREG: {
        EXPECT_TRUE(description.data() != NULL);
        EXPECT_EQ(sizeof(user_fpxregs_struct), description.length());
        ++num_nt_prxfpreg;
        break;
      }
#endif
      default:
        break;
    }
    note = note.GetNextNote();
  }

  EXPECT_TRUE(expected_thread_ids == actual_thread_ids);
  EXPECT_EQ(1U, num_nt_prpsinfo);
  EXPECT_EQ(kNumOfThreads, num_nt_prstatus);
#if defined(__i386__) || defined(__x86_64__)
  EXPECT_EQ(kNumOfThreads, num_nt_fpregset);
#endif
#if defined(__i386__)
  EXPECT_EQ(kNumOfThreads, num_nt_prxfpreg);
#endif
}
