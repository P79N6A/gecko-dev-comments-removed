































#include <string>

#include "breakpad_googletest_includes.h"
#include "google_breakpad/processor/network_source_line_resolver.h"
#include "google_breakpad/processor/stack_frame.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "processor/basic_code_module.h"
#include "processor/binarystream.h"
#include "processor/cfi_frame_info.h"
#include "processor/network_interface.h"
#include "processor/network_source_line_protocol.h"
#include "processor/windows_frame_info.h"

namespace google_breakpad {
class MockNetwork : public NetworkInterface {
 public:
  MockNetwork() {}

  MOCK_METHOD1(Init, bool(bool listen));
  MOCK_METHOD2(Send, bool(const char *data, size_t length));
  MOCK_METHOD1(WaitToReceive, bool(int timeout));
  MOCK_METHOD3(Receive, bool(char *buffer, size_t buffer_size,
                             ssize_t &received));
};
}

namespace {
using std::string;
using google_breakpad::binarystream;
using google_breakpad::BasicCodeModule;
using google_breakpad::CFIFrameInfo;
using google_breakpad::MockNetwork;
using google_breakpad::NetworkSourceLineResolver;
using google_breakpad::StackFrame;
using google_breakpad::SymbolSupplier;
using google_breakpad::WindowsFrameInfo;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using namespace google_breakpad::source_line_protocol;

class TestNetworkSourceLineResolver : public ::testing::Test {
public:
  TestNetworkSourceLineResolver() : resolver(NULL) {}

  void SetUp() {
    EXPECT_CALL(net, Init(false)).WillOnce(Return(true));
    resolver = new NetworkSourceLineResolver(&net, 0);
  }

  NetworkSourceLineResolver *resolver;
  MockNetwork net;
};

bool GeneratePositiveHasResponse(char *buffer, size_t buffer_size,
                                 ssize_t &received) {
  binarystream stream;
  stream << u_int16_t(0) << OK << MODULE_LOADED;
  string s = stream.str();
  received = s.length();
  memcpy(buffer, s.c_str(), s.length());
  return true;
}

bool GenerateNegativeHasResponse(char *buffer, size_t buffer_size,
                                 ssize_t &received) {
  binarystream stream;
  stream << u_int16_t(1) << OK << MODULE_NOT_LOADED;
  string s = stream.str();
  received = s.length();
  memcpy(buffer, s.c_str(), s.length());
  return true;
}

TEST_F(TestNetworkSourceLineResolver, TestHasMessages) {
  EXPECT_CALL(net, Send(_,_))
    .WillOnce(Return(true))
    .WillOnce(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .WillOnce(Return(true))
    .WillOnce(Return(true));
  EXPECT_CALL(net, Receive(_,_,_))
    .WillOnce(Invoke(GeneratePositiveHasResponse))
    .WillOnce(Invoke(GenerateNegativeHasResponse));
  ASSERT_NE(resolver, (NetworkSourceLineResolver*)NULL);
  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  EXPECT_TRUE(resolver->HasModule(&module));
  BasicCodeModule module2(0x0, 0x0, "test2.dll", "test2.pdb", "FFFF", "", "");
  EXPECT_FALSE(resolver->HasModule(&module2));
  
  EXPECT_TRUE(resolver->HasModule(&module));
}

bool GenerateErrorResponse(char *buffer, size_t buffer_size,
                           ssize_t &received) {
  binarystream stream;
  stream << u_int16_t(0) << ERROR;
  string s = stream.str();
  received = s.length();
  memcpy(buffer, s.c_str(), s.length());
  return true;
}

TEST_F(TestNetworkSourceLineResolver, TestHasErrorResponse) {
  EXPECT_CALL(net, Send(_,_))
    .WillOnce(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .WillOnce(Return(true));
  EXPECT_CALL(net, Receive(_,_,_))
    .WillOnce(Invoke(GenerateErrorResponse));
  ASSERT_NE(resolver, (NetworkSourceLineResolver*)NULL);
  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  
  EXPECT_FALSE(resolver->HasModule(&module));
}



class LoadHelper {
 public:
  LoadHelper() : response(LOAD_NOT_FOUND), sequence(0) {}
  bool GenerateLoadResponse(char *buffer, size_t buffer_size,
                            ssize_t &received) {
    binarystream stream;
    stream << sequence << OK << response;
    string s = stream.str();
    received = s.length();
    memcpy(buffer, s.c_str(), s.length());
    ++sequence;
    ++response;
    return true;
  }
  u_int8_t response;
  u_int16_t sequence;
};

TEST_F(TestNetworkSourceLineResolver, TestLoadMessages) {
  EXPECT_CALL(net, Send(_,_))
    .Times(4)
    .WillRepeatedly(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .Times(4)
    .WillRepeatedly(Return(true));
  LoadHelper helper;
  EXPECT_CALL(net, Receive(_,_,_))
    .Times(4)
    .WillRepeatedly(Invoke(&helper, &LoadHelper::GenerateLoadResponse));
  ASSERT_NE(resolver, (NetworkSourceLineResolver*)NULL);
  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  string s;
  EXPECT_EQ(SymbolSupplier::NOT_FOUND,
            resolver->GetSymbolFile(&module, NULL, &s));
  BasicCodeModule module2(0x0, 0x0, "test2.dll", "test2.pdb", "FFFF", "", "");
  EXPECT_EQ(SymbolSupplier::INTERRUPT,
            resolver->GetSymbolFile(&module2, NULL, &s));
  BasicCodeModule module3(0x0, 0x0, "test3.dll", "test3.pdb", "0000", "", "");
  
  EXPECT_EQ(SymbolSupplier::NOT_FOUND,
            resolver->GetSymbolFile(&module3, NULL, &s));
  BasicCodeModule module4(0x0, 0x0, "test4.dll", "test4.pdb", "1010", "", "");
  EXPECT_EQ(SymbolSupplier::FOUND,
            resolver->GetSymbolFile(&module4, NULL, &s));
  
  EXPECT_TRUE(resolver->HasModule(&module4));
}

TEST_F(TestNetworkSourceLineResolver, TestLoadErrorResponse) {
  EXPECT_CALL(net, Send(_,_))
    .WillOnce(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .WillOnce(Return(true));
  EXPECT_CALL(net, Receive(_,_,_))
    .WillOnce(Invoke(GenerateErrorResponse));
  ASSERT_NE(resolver, (NetworkSourceLineResolver*)NULL);
  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  string s;
  
  EXPECT_EQ(SymbolSupplier::NOT_FOUND,
            resolver->GetSymbolFile(&module, NULL, &s));
}

class GetHelper {
 public:
  GetHelper() : sequence(1) {}
  bool GenerateGetResponse(char *buffer, size_t buffer_size,
                           ssize_t &received) {
    binarystream stream;
    stream << sequence << OK;
    switch(sequence) {
    case 1:
      
      stream << string("test()") << u_int64_t(0x1000) << string("test.c")
             << u_int32_t(1) << u_int64_t(0x1010);
      break;
    case 2:
      
      stream << string("test2()") << u_int64_t(0x2000) << string("test2.c")
             << u_int32_t(2) << u_int64_t(0x2020);
      break;
    case 3:
      
      stream << string("test3()") << u_int64_t(0x4000) << string("")
             << u_int32_t(0) << u_int64_t(0);
      break;
    }
    string s = stream.str();
    received = s.length();
    memcpy(buffer, s.c_str(), s.length());
    ++sequence;
    return true;
  }
  u_int16_t sequence;
};

TEST_F(TestNetworkSourceLineResolver, TestGetMessages) {
  EXPECT_CALL(net, Send(_,_))
    .Times(4)
    .WillRepeatedly(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .Times(4)
    .WillRepeatedly(Return(true));
  GetHelper helper;
  EXPECT_CALL(net, Receive(_,_,_))
    .Times(4)
    .WillOnce(Invoke(GeneratePositiveHasResponse))
    .WillRepeatedly(Invoke(&helper, &GetHelper::GenerateGetResponse));
  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  
  
  EXPECT_TRUE(resolver->HasModule(&module));
  StackFrame frame;
  frame.module = &module;
  frame.instruction = 0x1010;
  resolver->FillSourceLineInfo(&frame);
  EXPECT_EQ("test()", frame.function_name);
  EXPECT_EQ(0x1000, frame.function_base);
  EXPECT_EQ("test.c", frame.source_file_name);
  EXPECT_EQ(1, frame.source_line);
  EXPECT_EQ(0x1010, frame.source_line_base);

  StackFrame frame2;
  frame2.module = &module;
  frame2.instruction = 0x2020;
  resolver->FillSourceLineInfo(&frame2);
  EXPECT_EQ("test2()", frame2.function_name);
  EXPECT_EQ(0x2000, frame2.function_base);
  EXPECT_EQ("test2.c", frame2.source_file_name);
  EXPECT_EQ(2, frame2.source_line);
  EXPECT_EQ(0x2020, frame2.source_line_base);

  StackFrame frame3;
  frame3.module = &module;
  frame3.instruction = 0x4040;
  resolver->FillSourceLineInfo(&frame3);
  EXPECT_EQ("test3()", frame3.function_name);
  EXPECT_EQ(0x4000, frame3.function_base);
  EXPECT_EQ("", frame3.source_file_name);
  EXPECT_EQ(0, frame3.source_line);
  EXPECT_EQ(0, frame3.source_line_base);

  
  StackFrame frame4;
  frame4.module = &module;
  frame4.instruction = 0x1010;
  resolver->FillSourceLineInfo(&frame4);
  EXPECT_EQ("test()", frame4.function_name);
  EXPECT_EQ(0x1000, frame4.function_base);
  EXPECT_EQ("test.c", frame4.source_file_name);
  EXPECT_EQ(1, frame4.source_line);
  EXPECT_EQ(0x1010, frame4.source_line_base);

  
  StackFrame frame5;
  frame5.module = &module;
  frame5.instruction = 0x4040;
  resolver->FillSourceLineInfo(&frame5);
  EXPECT_EQ("test3()", frame5.function_name);
  EXPECT_EQ(0x4000, frame5.function_base);
  EXPECT_EQ("", frame5.source_file_name);
  EXPECT_EQ(0, frame5.source_line);
  EXPECT_EQ(0, frame5.source_line_base);
}

class GetStackWinHelper {
 public:
  GetStackWinHelper() : sequence(1) {}
  bool GenerateGetStackWinResponse(char *buffer, size_t buffer_size,
                                   ssize_t &received) {
    binarystream stream;
    stream << sequence << OK;
    switch(sequence) {
    case 1:
      
      stream << string("0 0 0 1 2 3 a ff f00 1 x y =");
      break;
    case 2:
      
      stream << string("0 0 0 1 2 3 a ff f00 0 1");
      break;
    case 3:
      
      stream << string("");
      break;
    }
    string s = stream.str();
    received = s.length();
    memcpy(buffer, s.c_str(), s.length());
    ++sequence;
    return true;
  }
  u_int16_t sequence;
};

TEST_F(TestNetworkSourceLineResolver, TestGetStackWinMessages) {
  EXPECT_CALL(net, Send(_,_))
    .Times(4)
    .WillRepeatedly(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .Times(4)
    .WillRepeatedly(Return(true));
  GetStackWinHelper helper;
  EXPECT_CALL(net, Receive(_,_,_))
    .Times(4)
    .WillOnce(Invoke(GeneratePositiveHasResponse))
    .WillRepeatedly(Invoke(&helper,
                           &GetStackWinHelper::GenerateGetStackWinResponse));

  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  
  
  EXPECT_TRUE(resolver->HasModule(&module));
  StackFrame frame;
  frame.module = &module;
  frame.instruction = 0x1010;
  WindowsFrameInfo *info = resolver->FindWindowsFrameInfo(&frame);
  ASSERT_NE((WindowsFrameInfo*)NULL, info);
  EXPECT_EQ(0x1, info->prolog_size);
  EXPECT_EQ(0x2, info->epilog_size);
  EXPECT_EQ(0x3, info->parameter_size);
  EXPECT_EQ(0xa, info->saved_register_size);
  EXPECT_EQ(0xff, info->local_size);
  EXPECT_EQ(0xf00, info->max_stack_size);
  EXPECT_EQ("x y =", info->program_string);
  delete info;

  StackFrame frame2;
  frame2.module = &module;
  frame2.instruction = 0x2020;
  info = resolver->FindWindowsFrameInfo(&frame2);
  ASSERT_NE((WindowsFrameInfo*)NULL, info);
  EXPECT_EQ(0x1, info->prolog_size);
  EXPECT_EQ(0x2, info->epilog_size);
  EXPECT_EQ(0x3, info->parameter_size);
  EXPECT_EQ(0xa, info->saved_register_size);
  EXPECT_EQ(0xff, info->local_size);
  EXPECT_EQ(0xf00, info->max_stack_size);
  EXPECT_EQ("", info->program_string);
  EXPECT_EQ(true, info->allocates_base_pointer);
  delete info;

  StackFrame frame3;
  frame3.module = &module;
  frame3.instruction = 0x4040;
  info = resolver->FindWindowsFrameInfo(&frame3);
  EXPECT_EQ((WindowsFrameInfo*)NULL, info);

  
  StackFrame frame4;
  frame4.module = &module;
  frame4.instruction = 0x1010;
  info = resolver->FindWindowsFrameInfo(&frame4);
  ASSERT_NE((WindowsFrameInfo*)NULL, info);
  EXPECT_EQ(0x1, info->prolog_size);
  EXPECT_EQ(0x2, info->epilog_size);
  EXPECT_EQ(0x3, info->parameter_size);
  EXPECT_EQ(0xa, info->saved_register_size);
  EXPECT_EQ(0xff, info->local_size);
  EXPECT_EQ(0xf00, info->max_stack_size);
  EXPECT_EQ("x y =", info->program_string);
  delete info;

  
  StackFrame frame5;
  frame5.module = &module;
  frame5.instruction = 0x4040;
  info = resolver->FindWindowsFrameInfo(&frame5);
  EXPECT_EQ((WindowsFrameInfo*)NULL, info);
}

class GetStackCFIHelper {
 public:
  GetStackCFIHelper() : sequence(1) {}
  bool GenerateGetStackCFIResponse(char *buffer, size_t buffer_size,
                                   ssize_t &received) {
    binarystream stream;
    stream << sequence << OK;
    switch(sequence) {
    case 1:
      
      stream << string(".cfa: 1234 .ra: .cfa 5 + r0: abc xyz r2: 10 10");
      break;
    case 2:
      
      stream << string(".cfa: xyz");
      break;
    case 3:
      
      stream << string("");
      break;
    }
    string s = stream.str();
    received = s.length();
    memcpy(buffer, s.c_str(), s.length());
    ++sequence;
    return true;
  }
  u_int16_t sequence;
};

TEST_F(TestNetworkSourceLineResolver, TestGetStackCFIMessages) {
  EXPECT_CALL(net, Send(_,_))
    .Times(4)
    .WillRepeatedly(Return(true));
  EXPECT_CALL(net, WaitToReceive(0))
    .Times(4)
    .WillRepeatedly(Return(true));
  GetStackCFIHelper helper;
  EXPECT_CALL(net, Receive(_,_,_))
    .Times(4)
    .WillOnce(Invoke(GeneratePositiveHasResponse))
    .WillRepeatedly(Invoke(&helper,
                           &GetStackCFIHelper::GenerateGetStackCFIResponse));

  BasicCodeModule module(0x0, 0x0, "test.dll", "test.pdb", "ABCD", "", "");
  
  
  EXPECT_TRUE(resolver->HasModule(&module));
  StackFrame frame;
  frame.module = &module;
  frame.instruction = 0x1010;
  CFIFrameInfo *info = resolver->FindCFIFrameInfo(&frame);
  ASSERT_NE((CFIFrameInfo*)NULL, info);
  
  
  
  EXPECT_EQ(".cfa: 1234 .ra: .cfa 5 + r0: abc xyz r2: 10 10",
            info->Serialize());
  delete info;

  StackFrame frame2;
  frame2.module = &module;
  frame2.instruction = 0x2020;
  info = resolver->FindCFIFrameInfo(&frame2);
  ASSERT_NE((CFIFrameInfo*)NULL, info);
  EXPECT_EQ(".cfa: xyz", info->Serialize());
  delete info;

  StackFrame frame3;
  frame3.module = &module;
  frame3.instruction = 0x4040;
  info = resolver->FindCFIFrameInfo(&frame3);
  EXPECT_EQ((CFIFrameInfo*)NULL, info);

  
  StackFrame frame4;
  frame4.module = &module;
  frame4.instruction = 0x1010;
  info = resolver->FindCFIFrameInfo(&frame4);
  ASSERT_NE((CFIFrameInfo*)NULL, info);
  EXPECT_EQ(".cfa: 1234 .ra: .cfa 5 + r0: abc xyz r2: 10 10",
            info->Serialize());
  delete info;

  
  StackFrame frame5;
  frame5.module = &module;
  frame5.instruction = 0x4040;
  info = resolver->FindCFIFrameInfo(&frame5);
  EXPECT_EQ((CFIFrameInfo*)NULL, info);
}

TEST_F(TestNetworkSourceLineResolver, TestBogusData) {
  EXPECT_FALSE(resolver->HasModule(NULL));
  
  StackFrame frame;
  frame.module = NULL;
  frame.instruction = 0x1000;
  resolver->FillSourceLineInfo(&frame);
  EXPECT_EQ("", frame.function_name);
  EXPECT_EQ(0x0, frame.function_base);
  EXPECT_EQ("", frame.source_file_name);
  EXPECT_EQ(0, frame.source_line);
  EXPECT_EQ(0x0, frame.source_line_base);

  EXPECT_EQ((WindowsFrameInfo*)NULL, resolver->FindWindowsFrameInfo(&frame));
}

}  

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
