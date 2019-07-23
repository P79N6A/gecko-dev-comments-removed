
































#include "breakpad_googletest_includes.h"
#include "common/linux/dwarf_line_to_module.h"

using google_breakpad::DwarfLineToModule;
using google_breakpad::Module;
using google_breakpad::Module;

TEST(Simple, One) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("file1", 0x30bf0f27, 0, 0, 0);
  h.AddLine(0x6fd126fbf74f2680LL, 0x63c9a14cf556712bLL, 0x30bf0f27, 
            0x4c090cbf, 0x1cf9fe0d);

  vector<Module::File *> files;
  m.GetFiles(&files);
  EXPECT_EQ(1U, files.size());
  EXPECT_STREQ("file1", files[0]->name.c_str());

  EXPECT_EQ(1U, lines.size());
  EXPECT_EQ(0x6fd126fbf74f2680ULL, lines[0].address);
  EXPECT_EQ(0x63c9a14cf556712bULL, lines[0].size);
  EXPECT_TRUE(lines[0].file == files[0]);
  EXPECT_EQ(0x4c090cbf, lines[0].number);
}

TEST(Simple, Many) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory1", 0x838299ab);
  h.DefineDir("directory2", 0xf85de023);
  h.DefineFile("file1", 0x2b80377a, 0x838299ab, 0, 0);
  h.DefineFile("file1", 0x63beb4a4, 0xf85de023, 0, 0);
  h.DefineFile("file2", 0x1d161d56, 0x838299ab, 0, 0);
  h.DefineFile("file2", 0x1e7a667c, 0xf85de023, 0, 0);
  h.AddLine(0x69900c5d553b7274ULL, 0x90fded183f0d0d3cULL, 0x2b80377a,
            0x15b0f0a9U, 0x3ff5abd6U);
  h.AddLine(0x45811219a39b7101ULL, 0x25a5e6a924afc41fULL, 0x63beb4a4,
            0x4d259ce9U, 0x41c5ee32U);
  h.AddLine(0xfa90514c1dc9704bULL, 0x0063efeabc02f313ULL, 0x1d161d56,
            0x1ee9fa4fU, 0xbf70e46aU);
  h.AddLine(0x556b55fb6a647b10ULL, 0x3f3089ca2bfd80f5ULL, 0x1e7a667c,
            0x77fc280eU, 0x2c4a728cU);
  h.DefineFile("file3", -1, 0, 0, 0);
  h.AddLine(0xe2d72a37f8d9403aULL, 0x034dfab5b0d4d236ULL, 0x63beb4a5,
            0x75047044U, 0xb6a0016cU);

  vector<Module::File *> files;
  m.GetFiles(&files);
  ASSERT_EQ(5U, files.size());
  EXPECT_STREQ("directory1/file1", files[0]->name.c_str());
  EXPECT_STREQ("directory1/file2", files[1]->name.c_str());
  EXPECT_STREQ("directory2/file1", files[2]->name.c_str());
  EXPECT_STREQ("directory2/file2", files[3]->name.c_str());
  EXPECT_STREQ("file3",            files[4]->name.c_str());

  ASSERT_EQ(5U, lines.size());

  EXPECT_EQ(0x69900c5d553b7274ULL, lines[0].address);
  EXPECT_EQ(0x90fded183f0d0d3cULL, lines[0].size);
  EXPECT_TRUE(lines[0].file == files[0]);
  EXPECT_EQ(0x15b0f0a9, lines[0].number);

  EXPECT_EQ(0x45811219a39b7101ULL, lines[1].address);
  EXPECT_EQ(0x25a5e6a924afc41fULL, lines[1].size);
  EXPECT_TRUE(lines[1].file == files[2]);
  EXPECT_EQ(0x4d259ce9, lines[1].number);

  EXPECT_EQ(0xfa90514c1dc9704bULL, lines[2].address);
  EXPECT_EQ(0x0063efeabc02f313ULL, lines[2].size);
  EXPECT_TRUE(lines[2].file == files[1]);
  EXPECT_EQ(0x1ee9fa4f, lines[2].number);

  EXPECT_EQ(0x556b55fb6a647b10ULL, lines[3].address);
  EXPECT_EQ(0x3f3089ca2bfd80f5ULL, lines[3].size);
  EXPECT_TRUE(lines[3].file == files[3]);
  EXPECT_EQ(0x77fc280e, lines[3].number);

  EXPECT_EQ(0xe2d72a37f8d9403aULL, lines[4].address);
  EXPECT_EQ(0x034dfab5b0d4d236ULL, lines[4].size);
  EXPECT_TRUE(lines[4].file == files[4]);
  EXPECT_EQ(0x75047044, lines[4].number);
}

TEST(Filenames, Absolute) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory1", 1);
  h.DefineFile("/absolute", 1, 1, 0, 0);

  h.AddLine(1, 1, 1, 0, 0);

  vector<Module::File *> files;
  m.GetFiles(&files);
  ASSERT_EQ(1U, files.size());
  EXPECT_STREQ("/absolute", files[0]->name.c_str());
  ASSERT_EQ(1U, lines.size());
  EXPECT_TRUE(lines[0].file == files[0]);
}

TEST(Filenames, Relative) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory1", 1);
  h.DefineFile("relative", 1, 1, 0, 0);

  h.AddLine(1, 1, 1, 0, 0);

  vector<Module::File *> files;
  m.GetFiles(&files);
  ASSERT_EQ(1U, files.size());
  EXPECT_STREQ("directory1/relative", files[0]->name.c_str());
  ASSERT_EQ(1U, lines.size());
  EXPECT_TRUE(lines[0].file == files[0]);
}

TEST(Filenames, StrangeFile) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory1", 1);
  h.DefineFile("", 1, 1, 0, 0);
  h.AddLine(1, 1, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_STREQ("directory1/", lines[0].file->name.c_str());
}

TEST(Filenames, StrangeDirectory) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("", 1);
  h.DefineFile("file1", 1, 1, 0, 0);
  h.AddLine(1, 1, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_STREQ("/file1", lines[0].file->name.c_str());
}

TEST(Filenames, StrangeDirectoryAndFile) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("", 1);
  h.DefineFile("", 1, 1, 0, 0);
  h.AddLine(1, 1, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_STREQ("/", lines[0].file->name.c_str());
}



TEST(Errors, DirectoryZero) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory0", 0); 
  h.DefineFile("relative", 1, 0, 0, 0);

  h.AddLine(1, 1, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_STREQ("relative", lines[0].file->name.c_str());
}



TEST(Errors, BadFileNumber) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("relative", 1, 0, 0, 0);
  h.AddLine(1, 1, 2, 0, 0); 
  h.AddLine(2, 1, 2, 0, 0); 

  EXPECT_EQ(0U, lines.size());
}



TEST(Errors, BadDirectoryNumber) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineDir("directory1", 1);
  h.DefineFile("baddirnumber1", 1, 2, 0, 0); 
  h.DefineFile("baddirnumber2", 2, 2, 0, 0); 
  h.AddLine(1, 1, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_STREQ("baddirnumber1", lines[0].file->name.c_str());
}


TEST(Errors, EmptyLine) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(1, 0, 1, 0, 0);

  ASSERT_EQ(0U, lines.size());
}  



TEST(Errors, BigLine) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(0xffffffffffffffffULL, 2, 1, 0, 0);

  ASSERT_EQ(1U, lines.size());
  EXPECT_EQ(1U, lines[0].size);
}  






TEST(Omitted, DroppedThenGood) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(0,  10, 1, 83816211, 0);   
  h.AddLine(20, 10, 1, 13059195, 0);   

  ASSERT_EQ(1U, lines.size());
  EXPECT_EQ(13059195, lines[0].number);
}

TEST(Omitted, GoodThenDropped) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(0x9dd6a372, 10, 1, 41454594, 0);   
  h.AddLine(0,  10, 1, 44793413, 0);           

  ASSERT_EQ(1U, lines.size());
  EXPECT_EQ(41454594, lines[0].number);
}

TEST(Omitted, Mix1) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(0x679ed72f,  10,   1, 58932642, 0);   
  h.AddLine(0xdfb5a72d,  10,   1, 39847385, 0);   
  h.AddLine(0,           0x78, 1, 23053829, 0);   
  h.AddLine(0x78,        0x6a, 1, 65317783, 0);   
  h.AddLine(0x78 + 0x6a, 0x2a, 1, 77601423, 0);   
  h.AddLine(0x9fe0cea5,  10,   1, 91806582, 0);   
  h.AddLine(0x7e41a109,  10,   1, 56169221, 0);   

  ASSERT_EQ(4U, lines.size());
  EXPECT_EQ(58932642, lines[0].number);
  EXPECT_EQ(39847385, lines[1].number);
  EXPECT_EQ(91806582, lines[2].number);
  EXPECT_EQ(56169221, lines[3].number);
}

TEST(Omitted, Mix2) {
  Module m("name", "os", "architecture", "id");
  vector<Module::Line> lines;
  DwarfLineToModule h(&m, &lines);

  h.DefineFile("filename1", 1, 0, 0, 0);
  h.AddLine(0,           0xf2, 1, 58802211, 0);   
  h.AddLine(0xf2,        0xb9, 1, 78958222, 0);   
  h.AddLine(0xf2 + 0xb9, 0xf7, 1, 64861892, 0);   
  h.AddLine(0x4e4d271e,  9,    1, 67355743, 0);   
  h.AddLine(0xdfb5a72d,  30,   1, 23365776, 0);   
  h.AddLine(0,           0x64, 1, 76196762, 0);   
  h.AddLine(0x64,        0x33, 1, 71066611, 0);   
  h.AddLine(0x64 + 0x33, 0xe3, 1, 61749337, 0);   

  ASSERT_EQ(2U, lines.size());
  EXPECT_EQ(67355743, lines[0].number);
  EXPECT_EQ(23365776, lines[1].number);
}
