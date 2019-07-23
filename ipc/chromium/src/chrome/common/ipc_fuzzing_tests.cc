



#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#include "base/message_loop.h"
#include "base/platform_thread.h"
#include "base/process_util.h"
#include "chrome/common/ipc_channel.h"
#include "chrome/common/ipc_channel_proxy.h"
#include "chrome/common/ipc_message_utils.h"
#include "chrome/common/ipc_tests.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

TEST(IPCMessageIntegrity, ReadBeyondBufferStr) {
  
  uint32 v1 = kuint32max - 1;
  int v2 = 666;
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(v1));
  EXPECT_TRUE(m.WriteInt(v2));

  void* iter = NULL;
  std::string vs;
  EXPECT_FALSE(m.ReadString(&iter, &vs));
}

TEST(IPCMessageIntegrity, ReadBeyondBufferWStr) {
  
  uint32 v1 = kuint32max - 1;
  int v2 = 777;
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(v1));
  EXPECT_TRUE(m.WriteInt(v2));

  void* iter = NULL;
  std::wstring vs;
  EXPECT_FALSE(m.ReadWString(&iter, &vs));
}

TEST(IPCMessageIntegrity, ReadBytesBadIterator) {
  
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(1));
  EXPECT_TRUE(m.WriteInt(2));

  void* iter = NULL;
  const char* data = NULL;
  EXPECT_FALSE(m.ReadBytes(&iter, &data, sizeof(int)));
}

TEST(IPCMessageIntegrity, ReadVectorNegativeSize) {
  
  
  
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(-1));   
  EXPECT_TRUE(m.WriteInt(1));
  EXPECT_TRUE(m.WriteInt(2));
  EXPECT_TRUE(m.WriteInt(3));

  std::vector<double> vec;
  void* iter = 0;
  EXPECT_FALSE(ReadParam(&m, &iter, &vec));
}

TEST(IPCMessageIntegrity, ReadVectorTooLarge1) {
  
  
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(0x21000003));   
  EXPECT_TRUE(m.WriteInt64(1));
  EXPECT_TRUE(m.WriteInt64(2));

  std::vector<int64> vec;
  void* iter = 0;
  EXPECT_FALSE(ReadParam(&m, &iter, &vec));
}

TEST(IPCMessageIntegrity, ReadVectorTooLarge2) {
  
  
  
  IPC::Message m(0, 1, IPC::Message::PRIORITY_NORMAL);
  EXPECT_TRUE(m.WriteInt(0x71000000));   
  EXPECT_TRUE(m.WriteInt64(1));
  EXPECT_TRUE(m.WriteInt64(2));

  std::vector<int64> vec;
  void* iter = 0;
  EXPECT_FALSE(ReadParam(&m, &iter, &vec));
}



#define MESSAGES_INTERNAL_FILE "chrome/common/ipc_sync_message_unittest.h"
#include "chrome/common/ipc_message_macros.h"

enum IPCMessageIds {
  UNUSED_IPC_TYPE,
  SERVER_FIRST_IPC_TYPE,    
  SERVER_SECOND_IPC_TYPE,   
  SERVER_THIRD_IPC_TYPE,    
  CLIENT_MALFORMED_IPC,     
  CLIENT_UNHANDLED_IPC      
};


class MsgClassIS : public IPC::MessageWithTuple< Tuple2<int, std::wstring> > {
 public:
  enum { ID = SERVER_FIRST_IPC_TYPE };
  MsgClassIS(const int& arg1, const std::wstring& arg2)
      : IPC::MessageWithTuple< Tuple2<int, std::wstring> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(arg1, arg2)) {}
};


class MsgClassSI : public IPC::MessageWithTuple< Tuple2<std::wstring, int> > {
 public:
  enum { ID = SERVER_SECOND_IPC_TYPE };
  MsgClassSI(const std::wstring& arg1, const int& arg2)
      : IPC::MessageWithTuple< Tuple2<std::wstring, int> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(arg1, arg2)) {}
};


class MsgDoMutex : public IPC::MessageWithTuple< Tuple2<std::wstring, int> > {
 public:
  enum { ID = SERVER_THIRD_IPC_TYPE };
  MsgDoMutex(const std::wstring& mutex_name, const int& unused)
      : IPC::MessageWithTuple< Tuple2<std::wstring, int> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(mutex_name, unused)) {}
};

class SimpleListener : public IPC::Channel::Listener {
 public:
  SimpleListener() : other_(NULL) {
  }
  void Init(IPC::Message::Sender* s) {
    other_ = s;
  }
 protected:
  IPC::Message::Sender* other_;
};

enum {
  FUZZER_ROUTING_ID = 5
};




class FuzzerServerListener : public SimpleListener {
 public:
  FuzzerServerListener() : message_count_(2), pending_messages_(0) {
  }
  virtual void OnMessageReceived(const IPC::Message& msg) {
    if (msg.routing_id() == MSG_ROUTING_CONTROL) {
      ++pending_messages_;
      IPC_BEGIN_MESSAGE_MAP(FuzzerServerListener, msg)
        IPC_MESSAGE_HANDLER(MsgClassIS, OnMsgClassISMessage)
        IPC_MESSAGE_HANDLER(MsgClassSI, OnMsgClassSIMessage)
      IPC_END_MESSAGE_MAP()
      if (pending_messages_) {
        
        ReplyMsgNotHandled(msg.type());
      }
    }
  }

 private:
  void OnMsgClassISMessage(int value, const std::wstring& text) {
    UseData(MsgClassIS::ID, value, text);
    RoundtripAckReply(FUZZER_ROUTING_ID, MsgClassIS::ID, value);
    Cleanup();
  }

  void OnMsgClassSIMessage(const std::wstring& text, int value) {
    UseData(MsgClassSI::ID, value, text);
    RoundtripAckReply(FUZZER_ROUTING_ID, MsgClassSI::ID, value);
    Cleanup();
  }

  bool RoundtripAckReply(int routing, int type_id, int reply) {
    IPC::Message* message = new IPC::Message(routing, type_id,
                                             IPC::Message::PRIORITY_NORMAL);
    message->WriteInt(reply + 1);
    message->WriteInt(reply);
    return other_->Send(message);
  }

  void Cleanup() {
    --message_count_;
    --pending_messages_;
    if (0 == message_count_)
      MessageLoop::current()->Quit();
  }

  void ReplyMsgNotHandled(int type_id) {
    RoundtripAckReply(FUZZER_ROUTING_ID, CLIENT_UNHANDLED_IPC, type_id);
    Cleanup();
  }

  void UseData(int caller, int value, const std::wstring& text) {
    std::wostringstream wos;
    wos << L"IPC fuzzer:" << caller << " [" << value << L" " << text << L"]\n";
    std::wstring output = wos.str();
    LOG(WARNING) << output.c_str();
  };

  int message_count_;
  int pending_messages_;
};

class FuzzerClientListener : public SimpleListener {
 public:
  FuzzerClientListener() : last_msg_(NULL) {
  }

  virtual void OnMessageReceived(const IPC::Message& msg) {
    last_msg_ = new IPC::Message(msg);
    MessageLoop::current()->Quit();
  }

  bool ExpectMessage(int value, int type_id) {
    if (!MsgHandlerInternal(type_id))
      return false;
    int msg_value1 = 0;
    int msg_value2 = 0;
    void* iter = NULL;
    if (!last_msg_->ReadInt(&iter, &msg_value1))
      return false;
    if (!last_msg_->ReadInt(&iter, &msg_value2))
      return false;
    if ((msg_value2 + 1) != msg_value1)
      return false;
    if (msg_value2 != value)
      return false;

    delete last_msg_;
    last_msg_ = NULL;
    return true;
  }

  bool ExpectMsgNotHandled(int type_id) {
    return ExpectMessage(type_id, CLIENT_UNHANDLED_IPC);
  }

 private:
  bool MsgHandlerInternal(int type_id) {
    MessageLoop::current()->Run();
    if (NULL == last_msg_)
      return false;
    if (FUZZER_ROUTING_ID != last_msg_->routing_id())
      return false;
    return (type_id == last_msg_->type());
  };

  IPC::Message* last_msg_;
};



MULTIPROCESS_TEST_MAIN(RunFuzzServer) {
  MessageLoopForIO main_message_loop;
  FuzzerServerListener listener;
  IPC::Channel chan(kFuzzerChannel, IPC::Channel::MODE_CLIENT, &listener);
  chan.Connect();
  listener.Init(&chan);
  MessageLoop::current()->Run();
  return 0;
}

class IPCFuzzingTest : public IPCChannelTest {
};



TEST_F(IPCFuzzingTest, SanityTest) {
  FuzzerClientListener listener;
  IPC::Channel chan(kFuzzerChannel, IPC::Channel::MODE_SERVER,
                    &listener);
  base::ProcessHandle server_process = SpawnChild(FUZZER_SERVER, &chan);
  ASSERT_TRUE(server_process);
  PlatformThread::Sleep(1000);
  ASSERT_TRUE(chan.Connect());
  listener.Init(&chan);

  IPC::Message* msg = NULL;
  int value = 43;
  msg = new MsgClassIS(value, L"expect 43");
  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMessage(value, MsgClassIS::ID));

  msg = new MsgClassSI(L"expect 44", ++value);
  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMessage(value, MsgClassSI::ID));

  EXPECT_TRUE(base::WaitForSingleProcess(server_process, 5000));
  base::CloseProcessHandle(server_process);
}






#ifdef NDEBUG
TEST_F(IPCFuzzingTest, MsgBadPayloadShort) {
  FuzzerClientListener listener;
  IPC::Channel chan(kFuzzerChannel, IPC::Channel::MODE_SERVER,
                    &listener);
  base::ProcessHandle server_process = SpawnChild(FUZZER_SERVER, &chan);
  ASSERT_TRUE(server_process);
  PlatformThread::Sleep(1000);
  ASSERT_TRUE(chan.Connect());
  listener.Init(&chan);

  IPC::Message* msg = new IPC::Message(MSG_ROUTING_CONTROL, MsgClassIS::ID,
                                       IPC::Message::PRIORITY_NORMAL);
  msg->WriteInt(666);
  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMsgNotHandled(MsgClassIS::ID));

  msg = new MsgClassSI(L"expect one", 1);
  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMessage(1, MsgClassSI::ID));

  EXPECT_TRUE(base::WaitForSingleProcess(server_process, 5000));
  base::CloseProcessHandle(server_process);
}
#endif  






TEST_F(IPCFuzzingTest, MsgBadPayloadArgs) {
  FuzzerClientListener listener;
  IPC::Channel chan(kFuzzerChannel, IPC::Channel::MODE_SERVER,
                    &listener);
  base::ProcessHandle server_process = SpawnChild(FUZZER_SERVER, &chan);
  ASSERT_TRUE(server_process);
  PlatformThread::Sleep(1000);
  ASSERT_TRUE(chan.Connect());
  listener.Init(&chan);

  IPC::Message* msg = new IPC::Message(MSG_ROUTING_CONTROL, MsgClassSI::ID,
                                       IPC::Message::PRIORITY_NORMAL);
  msg->WriteWString(L"d");
  msg->WriteInt(0);
  msg->WriteInt(0x65);  

  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMessage(0, MsgClassSI::ID));

  
  
  msg = new MsgClassIS(3, L"expect three");
  chan.Send(msg);
  EXPECT_TRUE(listener.ExpectMessage(3, MsgClassIS::ID));

  EXPECT_TRUE(base::WaitForSingleProcess(server_process, 5000));
  base::CloseProcessHandle(server_process);
}


class ServerMacroExTest {
 public:
  ServerMacroExTest() : unhandled_msgs_(0) {
  }
  virtual bool OnMessageReceived(const IPC::Message& msg) {
    bool msg_is_ok = false;
    IPC_BEGIN_MESSAGE_MAP_EX(ServerMacroExTest, msg, msg_is_ok)
      IPC_MESSAGE_HANDLER(MsgClassIS, OnMsgClassISMessage)
      IPC_MESSAGE_HANDLER(MsgClassSI, OnMsgClassSIMessage)
      IPC_MESSAGE_UNHANDLED(++unhandled_msgs_)
    IPC_END_MESSAGE_MAP_EX()
    return msg_is_ok;
  }

  int unhandled_msgs() const {
    return unhandled_msgs_;
  }

 private:
  void OnMsgClassISMessage(int value, const std::wstring& text) {
  }
  void OnMsgClassSIMessage(const std::wstring& text, int value) {
  }

  int unhandled_msgs_;
};

TEST_F(IPCFuzzingTest, MsgMapExMacro) {
  IPC::Message* msg = NULL;
  ServerMacroExTest server;

  
  msg = new MsgClassIS(3, L"text3");
  EXPECT_TRUE(server.OnMessageReceived(*msg));
  delete msg;
  msg = new MsgClassSI(L"text2", 2);
  EXPECT_TRUE(server.OnMessageReceived(*msg));
  delete msg;

#ifdef NDEBUG
  
  msg = new IPC::Message(MSG_ROUTING_CONTROL, MsgClassSI::ID,
                         IPC::Message::PRIORITY_NORMAL);
  msg->WriteInt(2);
  EXPECT_FALSE(server.OnMessageReceived(*msg));
  delete msg;

  msg = new IPC::Message(MSG_ROUTING_CONTROL, MsgClassIS::ID,
                         IPC::Message::PRIORITY_NORMAL);
  msg->WriteInt(0x64);
  msg->WriteInt(0x32);
  EXPECT_FALSE(server.OnMessageReceived(*msg));
  delete msg;

  EXPECT_EQ(0, server.unhandled_msgs());
#endif
}
