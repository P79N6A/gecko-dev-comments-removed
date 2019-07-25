
































#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/stabs_to_module.h"

using google_breakpad::Module;
using google_breakpad::StabsToModule;
using std::vector;

TEST(StabsToModule, SimpleCU) {
  Module m("name", "os", "arch", "id");
  StabsToModule h(&m);

  
  
  EXPECT_TRUE(h.StartCompilationUnit("compilation-unit", 0x9f4d1271e50db93bLL,
                                     "build-directory"));
  EXPECT_TRUE(h.StartFunction("function", 0xfde4abbed390c394LL));
  EXPECT_TRUE(h.Line(0xfde4abbed390c394LL, "source-file-name", 174823314));
  EXPECT_TRUE(h.EndFunction(0xfde4abbed390c3a4LL));
  EXPECT_TRUE(h.EndCompilationUnit(0xfee4abbed390c3a4LL));
  h.Finalize();

  
  Module::File *file = m.FindExistingFile("source-file-name");
  ASSERT_TRUE(file != NULL);

  vector<Module::Function *> functions;
  m.GetFunctions(&functions, functions.end());
  ASSERT_EQ((size_t) 1, functions.size());
  Module::Function *function = functions[0];
  EXPECT_STREQ("function", function->name.c_str());
  EXPECT_EQ(0xfde4abbed390c394LL, function->address);
  EXPECT_EQ(0x10U, function->size);
  EXPECT_EQ(0U, function->parameter_size);
  ASSERT_EQ((size_t) 1, function->lines.size());
  Module::Line *line = &function->lines[0];
  EXPECT_EQ(0xfde4abbed390c394LL, line->address);
  EXPECT_EQ(0x10U, line->size); 
  EXPECT_TRUE(line->file == file);
  EXPECT_EQ(174823314, line->number);
}

TEST(InferSizes, LineSize) {
  Module m("name", "os", "arch", "id");
  StabsToModule h(&m);

  
  
  EXPECT_TRUE(h.StartCompilationUnit("compilation-unit", 0xb4513962eff94e92LL,
                                     "build-directory"));
  EXPECT_TRUE(h.StartFunction("function", 0xb4513962eff94e92LL));
  EXPECT_TRUE(h.Line(0xb4513962eff94e92LL, "source-file-name-1", 77396614));
  EXPECT_TRUE(h.Line(0xb4513963eff94e92LL, "source-file-name-2", 87660088));
  EXPECT_TRUE(h.EndFunction(0));  
  EXPECT_TRUE(h.EndCompilationUnit(0)); 
  EXPECT_TRUE(h.StartCompilationUnit("compilation-unit-2", 0xb4523963eff94e92LL,
                                     "build-directory-2")); 
  EXPECT_TRUE(h.EndCompilationUnit(0));
  h.Finalize();

  
  Module::File *file1 = m.FindExistingFile("source-file-name-1");
  ASSERT_TRUE(file1 != NULL);
  Module::File *file2 = m.FindExistingFile("source-file-name-2");
  ASSERT_TRUE(file2 != NULL);

  vector<Module::Function *> functions;
  m.GetFunctions(&functions, functions.end());
  ASSERT_EQ((size_t) 1, functions.size());

  Module::Function *function = functions[0];
  EXPECT_STREQ("function", function->name.c_str());
  EXPECT_EQ(0xb4513962eff94e92LL, function->address);
  EXPECT_EQ(0x1000100000000ULL, function->size); 
  EXPECT_EQ(0U, function->parameter_size);
  ASSERT_EQ((size_t) 2, function->lines.size());

  Module::Line *line1 = &function->lines[0];
  EXPECT_EQ(0xb4513962eff94e92LL, line1->address);
  EXPECT_EQ(0x100000000ULL, line1->size); 
  EXPECT_TRUE(line1->file == file1);
  EXPECT_EQ(77396614, line1->number);

  Module::Line *line2 = &function->lines[1];
  EXPECT_EQ(0xb4513963eff94e92LL, line2->address);
  EXPECT_EQ(0x1000000000000ULL, line2->size); 
  EXPECT_TRUE(line2->file == file2);
  EXPECT_EQ(87660088, line2->number);
}

TEST(FunctionNames, Mangled) {
  Module m("name", "os", "arch", "id");
  StabsToModule h(&m);

  
  EXPECT_TRUE(h.StartCompilationUnit("compilation-unit", 0xf2cfda63cef7f46cLL,
                                     "build-directory"));
  EXPECT_TRUE(h.StartFunction("_ZNSt6vectorIySaIyEE9push_backERKy",
                              0xf2cfda63cef7f46dLL));
  EXPECT_TRUE(h.EndFunction(0));
  EXPECT_TRUE(h.EndCompilationUnit(0));

  h.Finalize();

  
  Module::File *file = m.FindExistingFile("compilation-unit");
  ASSERT_TRUE(file != NULL);

  vector<Module::Function *> functions;
  m.GetFunctions(&functions, functions.end());
  ASSERT_EQ(1U, functions.size());

  Module::Function *function = functions[0];
  
  
  EXPECT_STREQ("std::vector<unsigned long long, "
               "std::allocator<unsigned long long> >::"
               "push_back(unsigned long long const&)",
               function->name.c_str());
  EXPECT_EQ(0xf2cfda63cef7f46dLL, function->address);
  EXPECT_LT(0U, function->size); 
  EXPECT_EQ(0U, function->parameter_size);
  ASSERT_EQ(0U, function->lines.size());
}





TEST(Omitted, Function) {
  Module m("name", "os", "arch", "id");
  StabsToModule h(&m);

  
  
  EXPECT_TRUE(h.StartCompilationUnit("compilation-unit", 0, "build-directory"));
  EXPECT_TRUE(h.StartFunction("function", 0x2a133596));
  EXPECT_TRUE(h.EndFunction(0));
  EXPECT_TRUE(h.EndCompilationUnit(0));
}



















