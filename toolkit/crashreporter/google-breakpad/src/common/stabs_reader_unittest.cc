
































#include <assert.h>
#include <errno.h>
#include <stab.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "breakpad_googletest_includes.h"
#include "common/stabs_reader.h"
#include "common/test_assembler.h"
#include "common/using_std_string.h"

using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::Test;
using ::testing::_;
using google_breakpad::StabsHandler;
using google_breakpad::StabsReader;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using google_breakpad::test_assembler::kBigEndian;
using google_breakpad::test_assembler::kLittleEndian;
using std::map;

namespace {



class StringAssembler: public Section {
 public:
  StringAssembler() : in_cu_(false) { StartCU(); }

  
  
  
  size_t Add(const string &s) {
    map<string, size_t>::iterator it = added_.find(s);
    if (it != added_.end())
      return it->second;
    size_t offset = Size() - cu_start_;
    AppendCString(s);
    added_[s] = offset;
    return offset;
  }

  
  void StartCU() {
    
    
    
    
    if (in_cu_) return;

    added_.clear();
    cu_start_ = Size();

    
    AppendCString("");
    added_[""] = 0;

    in_cu_ = true;
  }

  
  size_t EndCU() {
    assert(in_cu_);
    in_cu_ = false;
    return Size() - cu_start_;
  }

 private:
  
  size_t cu_start_;

  
  bool in_cu_;

  
  
  map<string, size_t> added_;
};



class StabsAssembler: public Section {
 public:
  
  StabsAssembler(StringAssembler *string_assembler)
      : Section(string_assembler->endianness()),
        string_assembler_(string_assembler),
        value_size_(0),
        entry_count_(0),
        cu_header_(NULL) { }
  ~StabsAssembler() { assert(!cu_header_); }

  
  size_t value_size() const { return value_size_; }
  StabsAssembler &set_value_size(size_t value_size) {
    value_size_ = value_size;
    return *this;
  }

  
  
  
  
  
  StabsAssembler &Stab(uint8_t type, uint8_t other, Label descriptor,
                       Label value, Label name) {
    D32(name);
    D8(type);
    D8(other);
    D16(descriptor);
    Append(endianness(), value_size_, value);
    entry_count_++;
    return *this;
  }

  
  StabsAssembler &Stab(uint8_t type, uint8_t other, Label descriptor,
                       Label value, const string &name) {
    return Stab(type, other, descriptor, value, string_assembler_->Add(name));
  }

  
  
  
  StabsAssembler &StartCU(const string &name) {
    assert(!cu_header_);
    cu_header_ = new CUHeader;
    string_assembler_->StartCU();
    entry_count_ = 0;
    return Stab(N_UNDF, 0,
                cu_header_->final_entry_count,
                cu_header_->final_string_size,
                string_assembler_->Add(name));
  }

  
  
  StabsAssembler &EndCU() {
    assert(cu_header_);
    cu_header_->final_entry_count = entry_count_;
    cu_header_->final_string_size = string_assembler_->EndCU();
    delete cu_header_;
    cu_header_ = NULL;
    return *this;
  }

 private:
  
  
  struct CUHeader {
    
    Label final_entry_count;

    
    Label final_string_size;
  };

  
  StringAssembler *string_assembler_;

  
  size_t value_size_;

  
  size_t entry_count_;

  
  
  CUHeader *cu_header_;
};

class MockStabsReaderHandler: public StabsHandler {
 public:
  MOCK_METHOD3(StartCompilationUnit,
               bool(const char *, uint64_t, const char *));
  MOCK_METHOD1(EndCompilationUnit, bool(uint64_t));
  MOCK_METHOD2(StartFunction, bool(const string &, uint64_t));
  MOCK_METHOD1(EndFunction, bool(uint64_t));
  MOCK_METHOD3(Line, bool(uint64_t, const char *, int));
  MOCK_METHOD2(Extern, bool(const string &, uint64_t));
  void Warning(const char *format, ...) { MockWarning(format); }
  MOCK_METHOD1(MockWarning, void(const char *));
};

struct StabsFixture {
  StabsFixture() : stabs(&strings), unitized(true) { }

  
  
  
  
  
  bool ApplyHandlerToMockStabsData() {
    string stabs_contents, stabstr_contents;
    if (!stabs.GetContents(&stabs_contents) ||
        !strings.GetContents(&stabstr_contents))
      return false;

    
    StabsReader reader(
        reinterpret_cast<const uint8_t *>(stabs_contents.data()),
        stabs_contents.size(),
        reinterpret_cast<const uint8_t *>(stabstr_contents.data()),
        stabstr_contents.size(),
        stabs.endianness() == kBigEndian, stabs.value_size(), unitized,
        &mock_handler);
    return reader.Process();
  }

  StringAssembler strings;
  StabsAssembler stabs;
  bool unitized;
  MockStabsReaderHandler mock_handler;
};

class Stabs: public StabsFixture, public Test { };

TEST_F(Stabs, MockStabsInput) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(4);
  stabs
      .Stab(N_SO,      149, 40232, 0x18a2a72bU, "builddir/")
      .Stab(N_FUN,      83, 50010, 0x91a5353fU,
            "not the SO with source file name we expected ")
      .Stab(N_SO,      165, 24791, 0xfe69d23cU, "")
      .Stab(N_SO,      184, 34178, 0xca4d883aU, "builddir1/")
      .Stab(N_SO,       83, 40859, 0xd2fe5df3U, "file1.c")
      .Stab(N_LSYM,    147, 39565, 0x60d4bb8aU, "not the FUN we're looking for")
      .Stab(N_FUN,     120, 50271, 0xa049f4b1U, "fun1")
      .Stab(N_BINCL,   150, 15694, 0xef65c659U,
            "something to ignore in a FUN body")
      .Stab(N_SLINE,   147,  4967, 0xd904b3f, "")
      .Stab(N_SOL,     177, 56135, 0xbd97b1dcU, "header.h")
      .Stab(N_SLINE,   130, 24610, 0x90f145b, "")
      .Stab(N_FUN,      45, 32441, 0xbf27cf93U,
            "fun2:some stabs type info here:to trim from the name")
      .Stab(N_SLINE,   138, 39002, 0x8148b87, "")
      .Stab(N_SOL,      60, 49318, 0x1d06e025U, "file1.c")
      .Stab(N_SLINE,    29, 52163, 0x6eebbb7, "")
      .Stab(N_SO,      167,  4647, 0xd04b7448U, "")
      .Stab(N_LSYM,     58, 37837, 0xe6b14d37U, "")
      .Stab(N_SO,      152,  7810, 0x11759f10U, "file3.c")
      .Stab(N_SO,      218, 12447, 0x11cfe4b5U, "");

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file1.c"), 0xd2fe5df3U,
                                     StrEq("builddir1/")))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun1"), 0xa049f4b1U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0xa049f4b1U + 0xd904b3f, StrEq("file1.c"), 4967))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0xa049f4b1U + 0x90f145b, StrEq("header.h"), 24610))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xbf27cf93U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun2"), 0xbf27cf93U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0xbf27cf93U + 0x8148b87, StrEq("header.h"), 39002))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0xbf27cf93U + 0x6eebbb7, StrEq("file1.c"), 52163))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xd04b7448U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0xd04b7448U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartCompilationUnit(StrEq("file3.c"),
                                                   0x11759f10U, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x11cfe4b5U))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, AbruptCU) {
  stabs.set_endianness(kBigEndian);
  stabs.set_value_size(4);
  stabs.Stab(N_SO, 177, 23446, 0xbf10d5e4, "file2-1.c");

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file2-1.c"), 0xbf10d5e4, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, AbruptFunction) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(8);
  stabs
      .Stab(N_SO,      218,   26631,   0xb83ddf10U, "file3-1.c")
      .Stab(N_FUN,     113,   24765,   0xbbd4a145U, "fun3_1");

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file3-1.c"), 0xb83ddf10U, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun3_1"), 0xbbd4a145U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, NoCU) {
  stabs.set_endianness(kBigEndian);
  stabs.set_value_size(8);
  stabs.Stab(N_SO, 161, 25673, 0x8f676e7bU, "build-directory/");

  EXPECT_CALL(mock_handler, StartCompilationUnit(_, _, _))
      .Times(0);
  EXPECT_CALL(mock_handler, StartFunction(_, _))
      .Times(0);

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, NoCUEnd) {
  stabs.set_endianness(kBigEndian);
  stabs.set_value_size(8);
  stabs
      .Stab(N_SO,      116,   58280,   0x2f7493c9U, "file5-1.c")
      .Stab(N_SO,      224,   23057,   0xf9f1d50fU, "file5-2.c");

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file5-1.c"), 0x2f7493c9U, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file5-2.c"), 0xf9f1d50fU, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}




TEST_F(Stabs, Unitized) {
  stabs.set_endianness(kBigEndian);
  stabs.set_value_size(4);
  stabs
      .StartCU("antimony")
      .Stab(N_SO,   49, 26043, 0x7e259f1aU, "antimony")
      .Stab(N_FUN, 101, 63253, 0x7fbcccaeU, "arsenic")
      .Stab(N_SO,  124, 37175, 0x80b0014cU, "")
      .EndCU()
      .StartCU("aluminum")
      .Stab(N_SO,   72, 23084, 0x86756839U, "aluminum")
      .Stab(N_FUN,  59,  3305, 0xa8e120b0U, "selenium")
      .Stab(N_SO,  178, 56949, 0xbffff983U, "")
      .EndCU();

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("antimony"), 0x7e259f1aU, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(Eq("arsenic"), 0x7fbcccaeU))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0x80b0014cU))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x80b0014cU))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("aluminum"), 0x86756839U, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(Eq("selenium"), 0xa8e120b0U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xbffff983U))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0xbffff983U))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}




TEST_F(Stabs, NonUnitized) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(4);
  unitized = false;
  stabs
      .Stab(N_UNDF,    21, 11551, 0x9bad2b2e, "")
      .Stab(N_UNDF,    21, 11551, 0x9bad2b2e, "")
      .Stab(N_SO,      71, 45139, 0x11a97352, "Tanzania")
      .Stab(N_SO,     221, 41976, 0x21a97352, "");

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("Tanzania"),
                                     0x11a97352, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x21a97352))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, FunctionEnd) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(8);
  stabs
      .Stab(N_SO,    102, 62362, 0x52a830d644cd6942ULL, "compilation unit")
      
      .Stab(N_FUN,   216, 38405, 0xbb5ab70ecdd23bfeULL, "function 1")
      
      
      .Stab(N_FUN,   240, 10973, 0xc954de9b8fb3e5e2ULL, "function 2")
      .Stab(N_FUN,    14, 36749, 0xc1ab,     "")
      
      .Stab(N_FUN,   143, 64514, 0xdff98c9a35386e1fULL, "function 3")
      .Stab(N_SO,    164, 60142, 0xfdacb856e78bbf57ULL, "");

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("compilation unit"),
                                     0x52a830d644cd6942ULL, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartFunction(Eq("function 1"), 0xbb5ab70ecdd23bfeULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xc954de9b8fb3e5e2ULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartFunction(Eq("function 2"), 0xc954de9b8fb3e5e2ULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xc954de9b8fb3e5e2ULL + 0xc1ab))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartFunction(Eq("function 3"), 0xdff98c9a35386e1fULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xfdacb856e78bbf57ULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0xfdacb856e78bbf57ULL))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}



TEST_F(Stabs, LeadingLine) {
  stabs.set_endianness(kBigEndian);
  stabs.set_value_size(4);
  stabs
      .Stab(N_SO,    179, 27357, 0x8adabc15, "build directory/")
      .Stab(N_SO,     52, 53058, 0x4c7e3bf4, "compilation unit")
      .Stab(N_SOL,   165, 12086, 0x6a797ca3, "source file name")
      .Stab(N_SLINE, 229, 20015, 0x4cb3d7e0, "")
      .Stab(N_SLINE,  89, 43802, 0x4cba8b88, "")
      .Stab(N_FUN,   251, 51639, 0xce1b98fa, "rutabaga")
      .Stab(N_FUN,   218, 16113, 0x5798,     "")
      .Stab(N_SO,     52, 53058, 0xd4af4415, "");

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("compilation unit"),
                                     0x4c7e3bf4, StrEq("build directory/")))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartFunction(Eq("rutabaga"), 0xce1b98fa))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0x4cb3d7e0, StrEq("source file name"), 20015))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Line(0x4cba8b88, StrEq("source file name"), 43802))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0xce1b98fa + 0x5798))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0xd4af4415))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}


#if defined(HAVE_MACH_O_NLIST_H)


TEST_F(Stabs, OnePublicSymbol) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(4);

  const u_int32_t kExpectedAddress = 0x9000;
  const string kExpectedFunctionName("public_function");
  stabs
    .Stab(N_SECT, 1, 0, kExpectedAddress, kExpectedFunctionName);

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                Extern(StrEq(kExpectedFunctionName),
                       kExpectedAddress))
        .WillOnce(Return(true));
  }
  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

TEST_F(Stabs, TwoPublicSymbols) {
  stabs.set_endianness(kLittleEndian);
  stabs.set_value_size(4);

  const u_int32_t kExpectedAddress1 = 0xB0B0B0B0;
  const string kExpectedFunctionName1("public_function");
  const u_int32_t kExpectedAddress2 = 0xF0F0F0F0;
  const string kExpectedFunctionName2("something else");
  stabs
    .Stab(N_SECT, 1, 0, kExpectedAddress1, kExpectedFunctionName1)
    .Stab(N_SECT, 1, 0, kExpectedAddress2, kExpectedFunctionName2);

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                Extern(StrEq(kExpectedFunctionName1),
                       kExpectedAddress1))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                Extern(StrEq(kExpectedFunctionName2),
                       kExpectedAddress2))
        .WillOnce(Return(true));
  }
  ASSERT_TRUE(ApplyHandlerToMockStabsData());
}

#endif

} 
