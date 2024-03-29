































#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/using_std_string.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/minidump.h"
#include "processor/logging.h"
#include "processor/synth_minidump.h"

namespace {

using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpMemoryInfo;
using google_breakpad::MinidumpMemoryInfoList;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpMemoryRegion;
using google_breakpad::MinidumpModule;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpThread;
using google_breakpad::MinidumpThreadList;
using google_breakpad::SynthMinidump::Context;
using google_breakpad::SynthMinidump::Dump;
using google_breakpad::SynthMinidump::Exception;
using google_breakpad::SynthMinidump::Memory;
using google_breakpad::SynthMinidump::Module;
using google_breakpad::SynthMinidump::Stream;
using google_breakpad::SynthMinidump::String;
using google_breakpad::SynthMinidump::SystemInfo;
using google_breakpad::SynthMinidump::Thread;
using google_breakpad::test_assembler::kBigEndian;
using google_breakpad::test_assembler::kLittleEndian;
using std::ifstream;
using std::istringstream;
using std::vector;
using ::testing::Return;

class MinidumpTest : public ::testing::Test {
public:
  void SetUp() {
    minidump_file_ = string(getenv("srcdir") ? getenv("srcdir") : ".") +
      "/src/processor/testdata/minidump2.dmp";
  }
  string minidump_file_;
};

TEST_F(MinidumpTest, TestMinidumpFromFile) {
  Minidump minidump(minidump_file_);
  ASSERT_EQ(minidump.path(), minidump_file_);
  ASSERT_TRUE(minidump.Read());
  const MDRawHeader* header = minidump.header();
  ASSERT_NE(header, (MDRawHeader*)NULL);
  ASSERT_EQ(header->signature, uint32_t(MD_HEADER_SIGNATURE));
  
}

TEST_F(MinidumpTest, TestMinidumpFromStream) {
  
  ifstream file_stream(minidump_file_.c_str(), std::ios::in);
  ASSERT_TRUE(file_stream.good());
  vector<char> bytes;
  file_stream.seekg(0, std::ios_base::end);
  ASSERT_TRUE(file_stream.good());
  bytes.resize(file_stream.tellg());
  file_stream.seekg(0, std::ios_base::beg);
  ASSERT_TRUE(file_stream.good());
  file_stream.read(&bytes[0], bytes.size());
  ASSERT_TRUE(file_stream.good());
  string str(&bytes[0], bytes.size());
  istringstream stream(str);
  ASSERT_TRUE(stream.good());

  
  Minidump minidump(stream);
  ASSERT_EQ(minidump.path(), "");
  ASSERT_TRUE(minidump.Read());
  const MDRawHeader* header = minidump.header();
  ASSERT_NE(header, (MDRawHeader*)NULL);
  ASSERT_EQ(header->signature, uint32_t(MD_HEADER_SIGNATURE));
  
}

TEST(Dump, ReadBackEmpty) {
  Dump dump(0);
  dump.Finish();
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream stream(contents);
  Minidump minidump(stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(0U, minidump.GetDirectoryEntryCount());
}

TEST(Dump, ReadBackEmptyBigEndian) {
  Dump big_minidump(0, kBigEndian);
  big_minidump.Finish();
  string contents;
  ASSERT_TRUE(big_minidump.GetContents(&contents));
  istringstream stream(contents);
  Minidump minidump(stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(0U, minidump.GetDirectoryEntryCount());
}

TEST(Dump, OneStream) {
  Dump dump(0, kBigEndian);
  Stream stream(dump, 0xfbb7fa2bU);
  stream.Append("stream contents");
  dump.Add(&stream);
  dump.Finish();
  
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  const MDRawDirectory *dir = minidump.GetDirectoryEntryAtIndex(0);
  ASSERT_TRUE(dir != NULL);
  EXPECT_EQ(0xfbb7fa2bU, dir->stream_type);

  uint32_t stream_length;
  ASSERT_TRUE(minidump.SeekToStreamType(0xfbb7fa2bU, &stream_length));
  ASSERT_EQ(15U, stream_length);
  char stream_contents[15];
  ASSERT_TRUE(minidump.ReadBytes(stream_contents, sizeof(stream_contents)));
  EXPECT_EQ(string("stream contents"),
            string(stream_contents, sizeof(stream_contents)));

  EXPECT_FALSE(minidump.GetThreadList());
  EXPECT_FALSE(minidump.GetModuleList());
  EXPECT_FALSE(minidump.GetMemoryList());
  EXPECT_FALSE(minidump.GetException());
  EXPECT_FALSE(minidump.GetAssertion());
  EXPECT_FALSE(minidump.GetSystemInfo());
  EXPECT_FALSE(minidump.GetMiscInfo());
  EXPECT_FALSE(minidump.GetBreakpadInfo());
}

TEST(Dump, OneMemory) {
  Dump dump(0, kBigEndian);
  Memory memory(dump, 0x309d68010bd21b2cULL);
  memory.Append("memory contents");
  dump.Add(&memory);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  const MDRawDirectory *dir = minidump.GetDirectoryEntryAtIndex(0);
  ASSERT_TRUE(dir != NULL);
  EXPECT_EQ((uint32_t) MD_MEMORY_LIST_STREAM, dir->stream_type);

  MinidumpMemoryList *memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(memory_list != NULL);
  ASSERT_EQ(1U, memory_list->region_count());

  MinidumpMemoryRegion *region1 = memory_list->GetMemoryRegionAtIndex(0);
  ASSERT_EQ(0x309d68010bd21b2cULL, region1->GetBase());
  ASSERT_EQ(15U, region1->GetSize());
  const uint8_t *region1_bytes = region1->GetMemory();
  ASSERT_TRUE(memcmp("memory contents", region1_bytes, 15) == 0);
}


TEST(Dump, OneThread) {
  Dump dump(0, kLittleEndian);
  Memory stack(dump, 0x2326a0fa);
  stack.Append("stack for thread");

  MDRawContextX86 raw_context;
  const uint32_t kExpectedEIP = 0x6913f540;
  raw_context.context_flags = MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL;
  raw_context.edi = 0x3ecba80d;
  raw_context.esi = 0x382583b9;
  raw_context.ebx = 0x7fccc03f;
  raw_context.edx = 0xf62f8ec2;
  raw_context.ecx = 0x46a6a6a8;
  raw_context.eax = 0x6a5025e2;
  raw_context.ebp = 0xd9fabb4a;
  raw_context.eip = kExpectedEIP;
  raw_context.cs = 0xbffe6eda;
  raw_context.eflags = 0xb2ce1e2d;
  raw_context.esp = 0x659caaa4;
  raw_context.ss = 0x2e951ef7;
  Context context(dump, raw_context);
  
  Thread thread(dump, 0xa898f11b, stack, context,
                0x9e39439f, 0x4abfc15f, 0xe499898a, 0x0d43e939dcfd0372ULL);
  
  dump.Add(&stack);
  dump.Add(&context);
  dump.Add(&thread);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(2U, minidump.GetDirectoryEntryCount());

  MinidumpMemoryList *md_memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(md_memory_list != NULL);
  ASSERT_EQ(1U, md_memory_list->region_count());

  MinidumpMemoryRegion *md_region = md_memory_list->GetMemoryRegionAtIndex(0);
  ASSERT_EQ(0x2326a0faU, md_region->GetBase());
  ASSERT_EQ(16U, md_region->GetSize());
  const uint8_t *region_bytes = md_region->GetMemory();
  ASSERT_TRUE(memcmp("stack for thread", region_bytes, 16) == 0);

  MinidumpThreadList *thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list != NULL);
  ASSERT_EQ(1U, thread_list->thread_count());

  MinidumpThread *md_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(md_thread != NULL);
  uint32_t thread_id;
  ASSERT_TRUE(md_thread->GetThreadID(&thread_id));
  ASSERT_EQ(0xa898f11bU, thread_id);
  MinidumpMemoryRegion *md_stack = md_thread->GetMemory();
  ASSERT_TRUE(md_stack != NULL);
  ASSERT_EQ(0x2326a0faU, md_stack->GetBase());
  ASSERT_EQ(16U, md_stack->GetSize());
  const uint8_t *md_stack_bytes = md_stack->GetMemory();
  ASSERT_TRUE(memcmp("stack for thread", md_stack_bytes, 16) == 0);

  MinidumpContext *md_context = md_thread->GetContext();
  ASSERT_TRUE(md_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_X86, md_context->GetContextCPU());

  uint64_t eip;
  ASSERT_TRUE(md_context->GetInstructionPointer(&eip));
  EXPECT_EQ(kExpectedEIP, eip);

  const MDRawContextX86 *md_raw_context = md_context->GetContextX86();
  ASSERT_TRUE(md_raw_context != NULL);
  ASSERT_EQ((uint32_t) (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL),
            (md_raw_context->context_flags
             & (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL)));
  EXPECT_EQ(0x3ecba80dU, raw_context.edi);
  EXPECT_EQ(0x382583b9U, raw_context.esi);
  EXPECT_EQ(0x7fccc03fU, raw_context.ebx);
  EXPECT_EQ(0xf62f8ec2U, raw_context.edx);
  EXPECT_EQ(0x46a6a6a8U, raw_context.ecx);
  EXPECT_EQ(0x6a5025e2U, raw_context.eax);
  EXPECT_EQ(0xd9fabb4aU, raw_context.ebp);
  EXPECT_EQ(kExpectedEIP, raw_context.eip);
  EXPECT_EQ(0xbffe6edaU, raw_context.cs);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.eflags);
  EXPECT_EQ(0x659caaa4U, raw_context.esp);
  EXPECT_EQ(0x2e951ef7U, raw_context.ss);
}

TEST(Dump, ThreadMissingMemory) {
  Dump dump(0, kLittleEndian);
  Memory stack(dump, 0x2326a0fa);
  

  MDRawContextX86 raw_context;
  memset(&raw_context, 0, sizeof(raw_context));
  raw_context.context_flags = MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL;
  Context context(dump, raw_context);

  Thread thread(dump, 0xa898f11b, stack, context,
                0x9e39439f, 0x4abfc15f, 0xe499898a, 0x0d43e939dcfd0372ULL);

  dump.Add(&stack);
  dump.Add(&context);
  dump.Add(&thread);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(2U, minidump.GetDirectoryEntryCount());

  
  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list != NULL);
  ASSERT_EQ(1U, thread_list->thread_count());

  MinidumpThread* md_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(md_thread != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_thread->GetThreadID(&thread_id));
  ASSERT_EQ(0xa898f11bU, thread_id);

  MinidumpContext* md_context = md_thread->GetContext();
  ASSERT_NE(reinterpret_cast<MinidumpContext*>(NULL), md_context);

  MinidumpMemoryRegion* md_stack = md_thread->GetMemory();
  ASSERT_EQ(reinterpret_cast<MinidumpMemoryRegion*>(NULL), md_stack);
}

TEST(Dump, ThreadMissingContext) {
  Dump dump(0, kLittleEndian);
  Memory stack(dump, 0x2326a0fa);
  stack.Append("stack for thread");

  
  Context context(dump);

  Thread thread(dump, 0xa898f11b, stack, context,
                0x9e39439f, 0x4abfc15f, 0xe499898a, 0x0d43e939dcfd0372ULL);

  dump.Add(&stack);
  dump.Add(&context);
  dump.Add(&thread);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(2U, minidump.GetDirectoryEntryCount());

  
  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list != NULL);
  ASSERT_EQ(1U, thread_list->thread_count());

  MinidumpThread* md_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(md_thread != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_thread->GetThreadID(&thread_id));
  ASSERT_EQ(0xa898f11bU, thread_id);
  MinidumpMemoryRegion* md_stack = md_thread->GetMemory();
  ASSERT_NE(reinterpret_cast<MinidumpMemoryRegion*>(NULL), md_stack);

  MinidumpContext* md_context = md_thread->GetContext();
  ASSERT_EQ(reinterpret_cast<MinidumpContext*>(NULL), md_context);
}

TEST(Dump, OneModule) {
  static const MDVSFixedFileInfo fixed_file_info = {
    0xb2fba33a,                           
    0x33d7a728,                           
    0x31afcb20,                           
    0xe51cdab1,                           
    0xd1ea6907,                           
    0x03032857,                           
    0x11bf71d7,                           
    0x5fb8cdbf,                           
    0xe45d0d5d,                           
    0x107d9562,                           
    0x5a8844d4,                           
    0xa8d30b20,                           
    0x651c3e4e                            
  };

  Dump dump(0, kBigEndian);
  String module_name(dump, "single module");
  Module module(dump, 0xa90206ca83eb2852ULL, 0xada542bd,
                module_name,
                0xb1054d2a,
                0x34571371,
                fixed_file_info, 
                NULL, NULL);

  dump.Add(&module);
  dump.Add(&module_name);
  dump.Finish();
  
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  const MDRawDirectory *dir = minidump.GetDirectoryEntryAtIndex(0);
  ASSERT_TRUE(dir != NULL);
  EXPECT_EQ((uint32_t) MD_MODULE_LIST_STREAM, dir->stream_type);

  MinidumpModuleList *md_module_list = minidump.GetModuleList();
  ASSERT_TRUE(md_module_list != NULL);
  ASSERT_EQ(1U, md_module_list->module_count());

  const MinidumpModule *md_module = md_module_list->GetModuleAtIndex(0);
  ASSERT_TRUE(md_module != NULL);
  ASSERT_EQ(0xa90206ca83eb2852ULL, md_module->base_address());
  ASSERT_EQ(0xada542bd, md_module->size());
  ASSERT_EQ("single module", md_module->code_file());

  const MDRawModule *md_raw_module = md_module->module();
  ASSERT_TRUE(md_raw_module != NULL);
  ASSERT_EQ(0xb1054d2aU, md_raw_module->time_date_stamp);
  ASSERT_EQ(0x34571371U, md_raw_module->checksum);
  ASSERT_TRUE(memcmp(&md_raw_module->version_info, &fixed_file_info,
                     sizeof(fixed_file_info)) == 0);
}

TEST(Dump, OneSystemInfo) {
  Dump dump(0, kLittleEndian);
  String csd_version(dump, "Petulant Pierogi");
  SystemInfo system_info(dump, SystemInfo::windows_x86, csd_version);

  dump.Add(&system_info);
  dump.Add(&csd_version);
  dump.Finish();
                         
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  const MDRawDirectory *dir = minidump.GetDirectoryEntryAtIndex(0);
  ASSERT_TRUE(dir != NULL);
  EXPECT_EQ((uint32_t) MD_SYSTEM_INFO_STREAM, dir->stream_type);

  MinidumpSystemInfo *md_system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(md_system_info != NULL);
  ASSERT_EQ("windows", md_system_info->GetOS());
  ASSERT_EQ("x86", md_system_info->GetCPU());
  ASSERT_EQ("Petulant Pierogi", *md_system_info->GetCSDVersion());
  ASSERT_EQ("GenuineIntel", *md_system_info->GetCPUVendor());
}

TEST(Dump, BigDump) {
  Dump dump(0, kLittleEndian);

  
  String csd_version(dump, "Munificent Macaque");
  SystemInfo system_info(dump, SystemInfo::windows_x86, csd_version);
  dump.Add(&csd_version);
  dump.Add(&system_info);

  
  Memory stack0(dump, 0x70b9ebfc);
  stack0.Append("stack for thread zero");
  MDRawContextX86 raw_context0;
  raw_context0.context_flags = MD_CONTEXT_X86_INTEGER;
  raw_context0.eip = 0xaf0709e4;
  Context context0(dump, raw_context0);
  Thread thread0(dump, 0xbbef4432, stack0, context0,
                 0xd0377e7b, 0xdb8eb0cf, 0xd73bc314, 0x09d357bac7f9a163ULL);
  dump.Add(&stack0);
  dump.Add(&context0);
  dump.Add(&thread0);

  Memory stack1(dump, 0xf988cc45);
  stack1.Append("stack for thread one");
  MDRawContextX86 raw_context1;
  raw_context1.context_flags = MD_CONTEXT_X86_INTEGER;
  raw_context1.eip = 0xe4f56f81;
  Context context1(dump, raw_context1);
  Thread thread1(dump, 0x657c3f58, stack1, context1,
                 0xa68fa182, 0x6f3cf8dd, 0xe3a78ccf, 0x78cc84775e4534bbULL);
  dump.Add(&stack1);
  dump.Add(&context1);
  dump.Add(&thread1);

  Memory stack2(dump, 0xc8a92e7c);
  stack2.Append("stack for thread two");
  MDRawContextX86 raw_context2;
  raw_context2.context_flags = MD_CONTEXT_X86_INTEGER;
  raw_context2.eip = 0xb336a438;
  Context context2(dump, raw_context2);
  Thread thread2(dump, 0xdf4b8a71, stack2, context2,
                 0x674c26b6, 0x445d7120, 0x7e700c56, 0xd89bf778e7793e17ULL);
  dump.Add(&stack2);
  dump.Add(&context2);
  dump.Add(&thread2);

  Memory stack3(dump, 0x36d08e08);
  stack3.Append("stack for thread three");
  MDRawContextX86 raw_context3;
  raw_context3.context_flags = MD_CONTEXT_X86_INTEGER;
  raw_context3.eip = 0xdf99a60c;
  Context context3(dump, raw_context3);
  Thread thread3(dump, 0x86e6c341, stack3, context3,
                 0x32dc5c55, 0x17a2aba8, 0xe0cc75e7, 0xa46393994dae83aeULL);
  dump.Add(&stack3);
  dump.Add(&context3);
  dump.Add(&thread3);

  Memory stack4(dump, 0x1e0ab4fa);
  stack4.Append("stack for thread four");
  MDRawContextX86 raw_context4;
  raw_context4.context_flags = MD_CONTEXT_X86_INTEGER;
  raw_context4.eip = 0xaa646267;
  Context context4(dump, raw_context4);
  Thread thread4(dump, 0x261a28d4, stack4, context4,
                 0x6ebd389e, 0xa0cd4759, 0x30168846, 0x164f650a0cf39d35ULL);
  dump.Add(&stack4);
  dump.Add(&context4);
  dump.Add(&thread4);

  
  String module1_name(dump, "module one");
  Module module1(dump, 0xeb77da57b5d4cbdaULL, 0x83cd5a37, module1_name);
  dump.Add(&module1_name);
  dump.Add(&module1);

  String module2_name(dump, "module two");
  Module module2(dump, 0x8675884adfe5ac90ULL, 0xb11e4ea3, module2_name);
  dump.Add(&module2_name);
  dump.Add(&module2);

  String module3_name(dump, "module three");
  Module module3(dump, 0x95fc1544da321b6cULL, 0x7c2bf081, module3_name);
  dump.Add(&module3_name);
  dump.Add(&module3);

  
  Memory memory5(dump, 0x61979e828040e564ULL);
  memory5.Append("contents of memory 5");
  dump.Add(&memory5);

  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(4U, minidump.GetDirectoryEntryCount());

  
  MinidumpThreadList *thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list != NULL);
  ASSERT_EQ(5U, thread_list->thread_count());
  uint32_t thread_id;
  ASSERT_TRUE(thread_list->GetThreadAtIndex(0)->GetThreadID(&thread_id));
  ASSERT_EQ(0xbbef4432U, thread_id);
  ASSERT_EQ(0x70b9ebfcU,
            thread_list->GetThreadAtIndex(0)->GetMemory()->GetBase());
  ASSERT_EQ(0xaf0709e4U,
            thread_list->GetThreadAtIndex(0)->GetContext()->GetContextX86()
            ->eip);

  ASSERT_TRUE(thread_list->GetThreadAtIndex(1)->GetThreadID(&thread_id));
  ASSERT_EQ(0x657c3f58U, thread_id);
  ASSERT_EQ(0xf988cc45U,
            thread_list->GetThreadAtIndex(1)->GetMemory()->GetBase());
  ASSERT_EQ(0xe4f56f81U,
            thread_list->GetThreadAtIndex(1)->GetContext()->GetContextX86()
            ->eip);

  ASSERT_TRUE(thread_list->GetThreadAtIndex(2)->GetThreadID(&thread_id));
  ASSERT_EQ(0xdf4b8a71U, thread_id);
  ASSERT_EQ(0xc8a92e7cU,
            thread_list->GetThreadAtIndex(2)->GetMemory()->GetBase());
  ASSERT_EQ(0xb336a438U,
            thread_list->GetThreadAtIndex(2)->GetContext()->GetContextX86()
            ->eip);

  ASSERT_TRUE(thread_list->GetThreadAtIndex(3)->GetThreadID(&thread_id));
  ASSERT_EQ(0x86e6c341U, thread_id);
  ASSERT_EQ(0x36d08e08U,
            thread_list->GetThreadAtIndex(3)->GetMemory()->GetBase());
  ASSERT_EQ(0xdf99a60cU,
            thread_list->GetThreadAtIndex(3)->GetContext()->GetContextX86()
            ->eip);

  ASSERT_TRUE(thread_list->GetThreadAtIndex(4)->GetThreadID(&thread_id));
  ASSERT_EQ(0x261a28d4U, thread_id);
  ASSERT_EQ(0x1e0ab4faU,
            thread_list->GetThreadAtIndex(4)->GetMemory()->GetBase());
  ASSERT_EQ(0xaa646267U,
            thread_list->GetThreadAtIndex(4)->GetContext()->GetContextX86()
            ->eip);

  
  MinidumpModuleList *md_module_list = minidump.GetModuleList();
  ASSERT_TRUE(md_module_list != NULL);
  ASSERT_EQ(3U, md_module_list->module_count());
  EXPECT_EQ(0xeb77da57b5d4cbdaULL,
            md_module_list->GetModuleAtIndex(0)->base_address());
  EXPECT_EQ(0x8675884adfe5ac90ULL,
            md_module_list->GetModuleAtIndex(1)->base_address());
  EXPECT_EQ(0x95fc1544da321b6cULL,
            md_module_list->GetModuleAtIndex(2)->base_address());
}

TEST(Dump, OneMemoryInfo) {
  Dump dump(0, kBigEndian);
  Stream stream(dump, MD_MEMORY_INFO_LIST_STREAM);

  
  const uint64_t kNumberOfEntries = 1;
  stream.D32(sizeof(MDRawMemoryInfoList))  
        .D32(sizeof(MDRawMemoryInfo))      
        .D64(kNumberOfEntries);            

  
  
  const uint64_t kBaseAddress = 0x1000;
  const uint64_t kRegionSize = 0x2000;
  stream.D64(kBaseAddress)                         
        .D64(kBaseAddress)                         
        .D32(MD_MEMORY_PROTECT_EXECUTE_READWRITE)  
        .D32(0)                                    
        .D64(kRegionSize)                          
        .D32(MD_MEMORY_STATE_COMMIT)               
        .D32(MD_MEMORY_PROTECT_EXECUTE_READWRITE)  
        .D32(MD_MEMORY_TYPE_PRIVATE)               
        .D32(0);                                   

  dump.Add(&stream);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  const MDRawDirectory *dir = minidump.GetDirectoryEntryAtIndex(0);
  ASSERT_TRUE(dir != NULL);
  EXPECT_EQ((uint32_t) MD_MEMORY_INFO_LIST_STREAM, dir->stream_type);

  MinidumpMemoryInfoList *info_list = minidump.GetMemoryInfoList();
  ASSERT_TRUE(info_list != NULL);
  ASSERT_EQ(1U, info_list->info_count());

  const MinidumpMemoryInfo *info1 = info_list->GetMemoryInfoAtIndex(0);
  ASSERT_EQ(kBaseAddress, info1->GetBase());
  ASSERT_EQ(kRegionSize, info1->GetSize());
  ASSERT_TRUE(info1->IsExecutable());
  ASSERT_TRUE(info1->IsWritable());

  
  const MinidumpMemoryInfo *info2 =
      info_list->GetMemoryInfoForAddress(kBaseAddress + kRegionSize / 2);
  ASSERT_EQ(kBaseAddress, info2->GetBase());
  ASSERT_EQ(kRegionSize, info2->GetSize());
}

TEST(Dump, OneExceptionX86) {
  Dump dump(0, kLittleEndian);

  MDRawContextX86 raw_context;
  raw_context.context_flags = MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL;
  raw_context.edi = 0x3ecba80d;
  raw_context.esi = 0x382583b9;
  raw_context.ebx = 0x7fccc03f;
  raw_context.edx = 0xf62f8ec2;
  raw_context.ecx = 0x46a6a6a8;
  raw_context.eax = 0x6a5025e2;
  raw_context.ebp = 0xd9fabb4a;
  raw_context.eip = 0x6913f540;
  raw_context.cs = 0xbffe6eda;
  raw_context.eflags = 0xb2ce1e2d;
  raw_context.esp = 0x659caaa4;
  raw_context.ss = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_TRUE(md_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_X86, md_context->GetContextCPU());
  const MDRawContextX86 *md_raw_context = md_context->GetContextX86();
  ASSERT_TRUE(md_raw_context != NULL);
  ASSERT_EQ((uint32_t) (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL),
            (md_raw_context->context_flags
             & (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL)));
  EXPECT_EQ(0x3ecba80dU, raw_context.edi);
  EXPECT_EQ(0x382583b9U, raw_context.esi);
  EXPECT_EQ(0x7fccc03fU, raw_context.ebx);
  EXPECT_EQ(0xf62f8ec2U, raw_context.edx);
  EXPECT_EQ(0x46a6a6a8U, raw_context.ecx);
  EXPECT_EQ(0x6a5025e2U, raw_context.eax);
  EXPECT_EQ(0xd9fabb4aU, raw_context.ebp);
  EXPECT_EQ(0x6913f540U, raw_context.eip);
  EXPECT_EQ(0xbffe6edaU, raw_context.cs);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.eflags);
  EXPECT_EQ(0x659caaa4U, raw_context.esp);
  EXPECT_EQ(0x2e951ef7U, raw_context.ss);
}

TEST(Dump, OneExceptionX86XState) {
  Dump dump(0, kLittleEndian);

  MDRawContextX86 raw_context;
  raw_context.context_flags = MD_CONTEXT_X86_INTEGER |
    MD_CONTEXT_X86_CONTROL | MD_CONTEXT_X86_XSTATE;
  raw_context.edi = 0x3ecba80d;
  raw_context.esi = 0x382583b9;
  raw_context.ebx = 0x7fccc03f;
  raw_context.edx = 0xf62f8ec2;
  raw_context.ecx = 0x46a6a6a8;
  raw_context.eax = 0x6a5025e2;
  raw_context.ebp = 0xd9fabb4a;
  raw_context.eip = 0x6913f540;
  raw_context.cs = 0xbffe6eda;
  raw_context.eflags = 0xb2ce1e2d;
  raw_context.esp = 0x659caaa4;
  raw_context.ss = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_TRUE(md_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_X86, md_context->GetContextCPU());
  const MDRawContextX86 *md_raw_context = md_context->GetContextX86();
  ASSERT_TRUE(md_raw_context != NULL);
  ASSERT_EQ((uint32_t) (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL),
            (md_raw_context->context_flags
             & (MD_CONTEXT_X86_INTEGER | MD_CONTEXT_X86_CONTROL)));
  EXPECT_EQ(0x3ecba80dU, raw_context.edi);
  EXPECT_EQ(0x382583b9U, raw_context.esi);
  EXPECT_EQ(0x7fccc03fU, raw_context.ebx);
  EXPECT_EQ(0xf62f8ec2U, raw_context.edx);
  EXPECT_EQ(0x46a6a6a8U, raw_context.ecx);
  EXPECT_EQ(0x6a5025e2U, raw_context.eax);
  EXPECT_EQ(0xd9fabb4aU, raw_context.ebp);
  EXPECT_EQ(0x6913f540U, raw_context.eip);
  EXPECT_EQ(0xbffe6edaU, raw_context.cs);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.eflags);
  EXPECT_EQ(0x659caaa4U, raw_context.esp);
  EXPECT_EQ(0x2e951ef7U, raw_context.ss);
}



TEST(Dump, OneExceptionX86NoCPUFlags) {
  Dump dump(0, kLittleEndian);

  MDRawContextX86 raw_context;
  
  raw_context.context_flags = 0;
  raw_context.edi = 0x3ecba80d;
  raw_context.esi = 0x382583b9;
  raw_context.ebx = 0x7fccc03f;
  raw_context.edx = 0xf62f8ec2;
  raw_context.ecx = 0x46a6a6a8;
  raw_context.eax = 0x6a5025e2;
  raw_context.ebp = 0xd9fabb4a;
  raw_context.eip = 0x6913f540;
  raw_context.cs = 0xbffe6eda;
  raw_context.eflags = 0xb2ce1e2d;
  raw_context.esp = 0x659caaa4;
  raw_context.ss = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);

  
  
  
  String csd_version(dump, "Service Pack 2");
  SystemInfo system_info(dump, SystemInfo::windows_x86, csd_version);
  dump.Add(&system_info);
  dump.Add(&csd_version);

  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(2U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_TRUE(md_context != NULL);

  ASSERT_EQ((uint32_t) MD_CONTEXT_X86, md_context->GetContextCPU());
  const MDRawContextX86 *md_raw_context = md_context->GetContextX86();
  ASSERT_TRUE(md_raw_context != NULL);

  
  
  
  ASSERT_EQ((uint32_t) (MD_CONTEXT_X86), md_raw_context->context_flags);

  EXPECT_EQ(0x3ecba80dU, raw_context.edi);
  EXPECT_EQ(0x382583b9U, raw_context.esi);
  EXPECT_EQ(0x7fccc03fU, raw_context.ebx);
  EXPECT_EQ(0xf62f8ec2U, raw_context.edx);
  EXPECT_EQ(0x46a6a6a8U, raw_context.ecx);
  EXPECT_EQ(0x6a5025e2U, raw_context.eax);
  EXPECT_EQ(0xd9fabb4aU, raw_context.ebp);
  EXPECT_EQ(0x6913f540U, raw_context.eip);
  EXPECT_EQ(0xbffe6edaU, raw_context.cs);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.eflags);
  EXPECT_EQ(0x659caaa4U, raw_context.esp);
  EXPECT_EQ(0x2e951ef7U, raw_context.ss);
}





TEST(Dump, OneExceptionX86NoCPUFlagsNoSystemInfo) {
  Dump dump(0, kLittleEndian);

  MDRawContextX86 raw_context;
  
  raw_context.context_flags = 0;
  raw_context.edi = 0x3ecba80d;
  raw_context.esi = 0x382583b9;
  raw_context.ebx = 0x7fccc03f;
  raw_context.edx = 0xf62f8ec2;
  raw_context.ecx = 0x46a6a6a8;
  raw_context.eax = 0x6a5025e2;
  raw_context.ebp = 0xd9fabb4a;
  raw_context.eip = 0x6913f540;
  raw_context.cs = 0xbffe6eda;
  raw_context.eflags = 0xb2ce1e2d;
  raw_context.esp = 0x659caaa4;
  raw_context.ss = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  
  
  
  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_EQ(NULL, md_context);
}

TEST(Dump, OneExceptionARM) {
  Dump dump(0, kLittleEndian);

  MDRawContextARM raw_context;
  raw_context.context_flags = MD_CONTEXT_ARM_INTEGER;
  raw_context.iregs[0] = 0x3ecba80d;
  raw_context.iregs[1] = 0x382583b9;
  raw_context.iregs[2] = 0x7fccc03f;
  raw_context.iregs[3] = 0xf62f8ec2;
  raw_context.iregs[4] = 0x46a6a6a8;
  raw_context.iregs[5] = 0x6a5025e2;
  raw_context.iregs[6] = 0xd9fabb4a;
  raw_context.iregs[7] = 0x6913f540;
  raw_context.iregs[8] = 0xbffe6eda;
  raw_context.iregs[9] = 0xb2ce1e2d;
  raw_context.iregs[10] = 0x659caaa4;
  raw_context.iregs[11] = 0xf0e0d0c0;
  raw_context.iregs[12] = 0xa9b8c7d6;
  raw_context.iregs[13] = 0x12345678;
  raw_context.iregs[14] = 0xabcd1234;
  raw_context.iregs[15] = 0x10203040;
  raw_context.cpsr = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_TRUE(md_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_ARM, md_context->GetContextCPU());
  const MDRawContextARM *md_raw_context = md_context->GetContextARM();
  ASSERT_TRUE(md_raw_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_ARM_INTEGER,
            (md_raw_context->context_flags
             & MD_CONTEXT_ARM_INTEGER));
  EXPECT_EQ(0x3ecba80dU, raw_context.iregs[0]);
  EXPECT_EQ(0x382583b9U, raw_context.iregs[1]);
  EXPECT_EQ(0x7fccc03fU, raw_context.iregs[2]);
  EXPECT_EQ(0xf62f8ec2U, raw_context.iregs[3]);
  EXPECT_EQ(0x46a6a6a8U, raw_context.iregs[4]);
  EXPECT_EQ(0x6a5025e2U, raw_context.iregs[5]);
  EXPECT_EQ(0xd9fabb4aU, raw_context.iregs[6]);
  EXPECT_EQ(0x6913f540U, raw_context.iregs[7]);
  EXPECT_EQ(0xbffe6edaU, raw_context.iregs[8]);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.iregs[9]);
  EXPECT_EQ(0x659caaa4U, raw_context.iregs[10]);
  EXPECT_EQ(0xf0e0d0c0U, raw_context.iregs[11]);
  EXPECT_EQ(0xa9b8c7d6U, raw_context.iregs[12]);
  EXPECT_EQ(0x12345678U, raw_context.iregs[13]);
  EXPECT_EQ(0xabcd1234U, raw_context.iregs[14]);
  EXPECT_EQ(0x10203040U, raw_context.iregs[15]);
  EXPECT_EQ(0x2e951ef7U, raw_context.cpsr);
}

TEST(Dump, OneExceptionARMOldFlags) {
  Dump dump(0, kLittleEndian);

  MDRawContextARM raw_context;
  
  raw_context.context_flags = MD_CONTEXT_ARM_OLD | 0x00000002;
  raw_context.iregs[0] = 0x3ecba80d;
  raw_context.iregs[1] = 0x382583b9;
  raw_context.iregs[2] = 0x7fccc03f;
  raw_context.iregs[3] = 0xf62f8ec2;
  raw_context.iregs[4] = 0x46a6a6a8;
  raw_context.iregs[5] = 0x6a5025e2;
  raw_context.iregs[6] = 0xd9fabb4a;
  raw_context.iregs[7] = 0x6913f540;
  raw_context.iregs[8] = 0xbffe6eda;
  raw_context.iregs[9] = 0xb2ce1e2d;
  raw_context.iregs[10] = 0x659caaa4;
  raw_context.iregs[11] = 0xf0e0d0c0;
  raw_context.iregs[12] = 0xa9b8c7d6;
  raw_context.iregs[13] = 0x12345678;
  raw_context.iregs[14] = 0xabcd1234;
  raw_context.iregs[15] = 0x10203040;
  raw_context.cpsr = 0x2e951ef7;
  Context context(dump, raw_context);

  Exception exception(dump, context,
                      0x1234abcd, 
                      0xdcba4321, 
                      0xf0e0d0c0, 
                      0x0919a9b9c9d9e9f9ULL); 
  
  dump.Add(&context);
  dump.Add(&exception);
  dump.Finish();

  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));

  istringstream minidump_stream(contents);
  Minidump minidump(minidump_stream);
  ASSERT_TRUE(minidump.Read());
  ASSERT_EQ(1U, minidump.GetDirectoryEntryCount());

  MinidumpException *md_exception = minidump.GetException();
  ASSERT_TRUE(md_exception != NULL);

  uint32_t thread_id;
  ASSERT_TRUE(md_exception->GetThreadID(&thread_id));
  ASSERT_EQ(0x1234abcdU, thread_id);

  const MDRawExceptionStream* raw_exception = md_exception->exception();
  ASSERT_TRUE(raw_exception != NULL);
  EXPECT_EQ(0xdcba4321, raw_exception->exception_record.exception_code);
  EXPECT_EQ(0xf0e0d0c0, raw_exception->exception_record.exception_flags);
  EXPECT_EQ(0x0919a9b9c9d9e9f9ULL,
            raw_exception->exception_record.exception_address);

  MinidumpContext *md_context = md_exception->GetContext();
  ASSERT_TRUE(md_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_ARM, md_context->GetContextCPU());
  const MDRawContextARM *md_raw_context = md_context->GetContextARM();
  ASSERT_TRUE(md_raw_context != NULL);
  ASSERT_EQ((uint32_t) MD_CONTEXT_ARM_INTEGER,
            (md_raw_context->context_flags
             & MD_CONTEXT_ARM_INTEGER));
  EXPECT_EQ(0x3ecba80dU, raw_context.iregs[0]);
  EXPECT_EQ(0x382583b9U, raw_context.iregs[1]);
  EXPECT_EQ(0x7fccc03fU, raw_context.iregs[2]);
  EXPECT_EQ(0xf62f8ec2U, raw_context.iregs[3]);
  EXPECT_EQ(0x46a6a6a8U, raw_context.iregs[4]);
  EXPECT_EQ(0x6a5025e2U, raw_context.iregs[5]);
  EXPECT_EQ(0xd9fabb4aU, raw_context.iregs[6]);
  EXPECT_EQ(0x6913f540U, raw_context.iregs[7]);
  EXPECT_EQ(0xbffe6edaU, raw_context.iregs[8]);
  EXPECT_EQ(0xb2ce1e2dU, raw_context.iregs[9]);
  EXPECT_EQ(0x659caaa4U, raw_context.iregs[10]);
  EXPECT_EQ(0xf0e0d0c0U, raw_context.iregs[11]);
  EXPECT_EQ(0xa9b8c7d6U, raw_context.iregs[12]);
  EXPECT_EQ(0x12345678U, raw_context.iregs[13]);
  EXPECT_EQ(0xabcd1234U, raw_context.iregs[14]);
  EXPECT_EQ(0x10203040U, raw_context.iregs[15]);
  EXPECT_EQ(0x2e951ef7U, raw_context.cpsr);
}

}  
