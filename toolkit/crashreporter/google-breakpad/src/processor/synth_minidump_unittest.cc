

































#include <sstream>
#include <string>

#include "breakpad_googletest_includes.h"
#include "google_breakpad/common/minidump_format.h"
#include "processor/synth_minidump.h"
#include "processor/synth_minidump_unittest_data.h"

using google_breakpad::SynthMinidump::Context;
using google_breakpad::SynthMinidump::Dump;
using google_breakpad::SynthMinidump::List;
using google_breakpad::SynthMinidump::Memory;
using google_breakpad::SynthMinidump::Module;
using google_breakpad::SynthMinidump::Section;
using google_breakpad::SynthMinidump::Stream;
using google_breakpad::SynthMinidump::String;
using google_breakpad::SynthMinidump::SystemInfo;
using google_breakpad::SynthMinidump::Thread;
using google_breakpad::TestAssembler::kBigEndian;
using google_breakpad::TestAssembler::kLittleEndian;
using google_breakpad::TestAssembler::Label;
using std::string;

TEST(Section, Simple) {
  Dump dump(0);
  Section section(dump);
  section.L32(0x12345678);
  section.Finish(0);
  string contents;
  ASSERT_TRUE(section.GetContents(&contents));
  EXPECT_EQ(string("\x78\x56\x34\x12", 4), contents);
}

TEST(Section, CiteLocationIn) {
  Dump dump(0, kBigEndian);
  Section section1(dump), section2(dump);
  section1.Append("order");
  section2.Append("mayhem");
  section2.Finish(0x32287ec2);
  section2.CiteLocationIn(&section1);
  string contents;
  ASSERT_TRUE(section1.GetContents(&contents));
  string expected("order\0\0\0\x06\x32\x28\x7e\xc2", 13);
  EXPECT_EQ(expected, contents);
}

TEST(Stream, CiteStreamIn) {
  Dump dump(0, kLittleEndian);
  Stream stream(dump, 0x40cae2b3);
  Section section(dump);
  stream.Append("stream contents");
  section.Append("section contents");
  stream.Finish(0x41424344);
  stream.CiteStreamIn(&section);
  string contents;
  ASSERT_TRUE(section.GetContents(&contents));
  string expected("section contents"
                  "\xb3\xe2\xca\x40"
                  "\x0f\0\0\0"
                  "\x44\x43\x42\x41",
                  16 + 4 + 4 + 4);
  EXPECT_EQ(expected, contents);
}

TEST(Memory, CiteMemoryIn) {
  Dump dump(0, kBigEndian);
  Memory memory(dump, 0x76d010874ab019f9ULL);
  Section section(dump);
  memory.Append("memory contents");
  section.Append("section contents");
  memory.Finish(0x51525354);
  memory.CiteMemoryIn(&section);
  string contents;
  ASSERT_TRUE(section.GetContents(&contents));
  string expected("section contents"
                  "\x76\xd0\x10\x87\x4a\xb0\x19\xf9"
                  "\0\0\0\x0f"
                  "\x51\x52\x53\x54",
                  16 + 8 + 4 + 4);
  EXPECT_EQ(contents, expected);
}

TEST(Memory, Here) {
  Dump dump(0, kBigEndian);
  Memory memory(dump, 0x89979731eb060ed4ULL);
  memory.Append(1729, 42);
  Label l = memory.Here();
  ASSERT_EQ(0x89979731eb060ed4ULL + 1729, l.Value());
}

TEST(Context, X86) {
  Dump dump(0, kLittleEndian);
  assert(x86_raw_context.context_flags & MD_CONTEXT_X86);
  Context context(dump, x86_raw_context);
  string contents;
  ASSERT_TRUE(context.GetContents(&contents));
  EXPECT_EQ(sizeof(x86_expected_contents), contents.size());
  EXPECT_TRUE(memcmp(contents.data(), x86_expected_contents, contents.size())
              == 0);
}

TEST(ContextDeathTest, X86BadFlags) {
  Dump dump(0, kLittleEndian);
  MDRawContextX86 raw;
  raw.context_flags = 0;
  ASSERT_DEATH(Context context(dump, raw);,
               "context\\.context_flags & 0x[0-9a-f]+");
}

TEST(ContextDeathTest, X86BadEndianness) {
  Dump dump(0, kBigEndian);
  MDRawContextX86 raw;
  raw.context_flags = MD_CONTEXT_X86;
  ASSERT_DEATH(Context context(dump, raw);,
               "dump\\.endianness\\(\\) == kLittleEndian");
}

TEST(Thread, Simple) {
  Dump dump(0, kLittleEndian);
  Context context(dump, x86_raw_context);
  context.Finish(0x8665da0c);
  Memory stack(dump, 0xaad55a93cc3c0efcULL);
  stack.Append("stack contents");
  stack.Finish(0xe08cdbd1);
  Thread thread(dump, 0x3d7ec360, stack, context,
                0x3593f44d, 
                0xab352b82, 
                0x2753d838, 
                0xeb2de4be3f29e3e9ULL); 
  string contents;
  ASSERT_TRUE(thread.GetContents(&contents));
  static const u_int8_t expected_bytes[] = {
    0x60, 0xc3, 0x7e, 0x3d, 
    0x4d, 0xf4, 0x93, 0x35, 
    0x82, 0x2b, 0x35, 0xab, 
    0x38, 0xd8, 0x53, 0x27, 
    0xe9, 0xe3, 0x29, 0x3f, 0xbe, 0xe4, 0x2d, 0xeb, 
    0xfc, 0x0e, 0x3c, 0xcc, 0x93, 0x5a, 0xd5, 0xaa, 
    0x0e, 0x00, 0x00, 0x00, 
    0xd1, 0xdb, 0x8c, 0xe0, 
    0xcc, 0x02, 0x00, 0x00, 
    0x0c, 0xda, 0x65, 0x86  
  };
  EXPECT_EQ(sizeof(expected_bytes), contents.size());
  EXPECT_TRUE(memcmp(contents.data(), expected_bytes, contents.size()) == 0);
}

TEST(String, Simple) {
  Dump dump(0, kBigEndian);
  String s(dump, "All mimsy were the borogoves");
  string contents;
  ASSERT_TRUE(s.GetContents(&contents));
  static const char expected[] = 
    "\x00\x00\x00\x38\0A\0l\0l\0 \0m\0i\0m\0s\0y\0 \0w\0e\0r\0e"
    "\0 \0t\0h\0e\0 \0b\0o\0r\0o\0g\0o\0v\0e\0s";
  string expected_string(expected, sizeof(expected) - 1);
  EXPECT_EQ(expected_string, contents);
}

TEST(String, CiteStringIn) {
  Dump dump(0, kLittleEndian);
  String s(dump, "and the mome wraths outgrabe");
  Section section(dump);
  section.Append("initial");
  s.CiteStringIn(&section);
  s.Finish(0xdc2bb469);
  string contents;
  ASSERT_TRUE(section.GetContents(&contents));
  EXPECT_EQ(string("initial\x69\xb4\x2b\xdc", 7 + 4), contents);
}

TEST(List, Empty) {
  Dump dump(0, kBigEndian);
  List<Section> list(dump, 0x2442779c);
  EXPECT_TRUE(list.Empty());
  list.Finish(0x84e09808);
  string contents;
  ASSERT_TRUE(list.GetContents(&contents));
  EXPECT_EQ(string("\0\0\0\0", 4), contents);
}

TEST(List, Two) {
  Dump dump(0, kBigEndian);
  List<Section> list(dump, 0x26c9f498);
  Section section1(dump);
  section1.Append("section one contents");
  EXPECT_TRUE(list.Empty());
  list.Add(&section1);
  EXPECT_FALSE(list.Empty());
  Section section2(dump);
  section2.Append("section two contents");
  list.Add(&section2);
  list.Finish(0x1e5bb60e);
  string contents;
  ASSERT_TRUE(list.GetContents(&contents));
  EXPECT_EQ(string("\0\0\0\x02section one contentssection two contents", 44),
            contents);
}

TEST(Dump, Header) {
  Dump dump(0x9f738b33685cc84cULL, kLittleEndian, 0xb3817faf, 0x2c741c0a);
  dump.Finish();
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  ASSERT_EQ(string("\x4d\x44\x4d\x50"   
                   "\xaf\x7f\x81\xb3"   
                   "\0\0\0\0"           
                   "\x20\0\0\0"         
                   "\0\0\0\0"           
                   "\x0a\x1c\x74\x2c"   
                   "\x4c\xc8\x5c\x68\x33\x8b\x73\x9f", 
                   32),
            contents);
}

TEST(Dump, HeaderBigEndian) {
  Dump dump(0x206ce3cc6fb8e0f0ULL, kBigEndian, 0x161693e2, 0x35667744);
  dump.Finish();
  string contents;
  ASSERT_TRUE(dump.GetContents(&contents));
  ASSERT_EQ(string("\x50\x4d\x44\x4d"   
                   "\x16\x16\x93\xe2"   
                   "\0\0\0\0"           
                   "\0\0\0\x20"         
                   "\0\0\0\0"           
                   "\x35\x66\x77\x44"   
                   "\x20\x6c\xe3\xcc\x6f\xb8\xe0\xf0", 
                   32),
            contents);
}

TEST(Dump, OneSection) {
  Dump dump(0, kLittleEndian);
  Section section(dump);
  section.Append("section contents");
  dump.Add(&section);
  dump.Finish();
  string dump_contents;
  
  ASSERT_TRUE(dump.GetContents(&dump_contents));

  Section referencing_section(dump);
  section.CiteLocationIn(&referencing_section);
  string contents;
  ASSERT_TRUE(referencing_section.GetContents(&contents));
  ASSERT_EQ(string("\x10\0\0\0\x20\0\0\0", 8), contents);
}
