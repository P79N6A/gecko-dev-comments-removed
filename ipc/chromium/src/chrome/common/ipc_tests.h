



#ifndef CHROME_COMMON_IPC_TESTS_H__
#define CHROME_COMMON_IPC_TESTS_H__

#include "base/multiprocess_test.h"
#include "base/process.h"



enum ChildType {
  TEST_CLIENT,
  TEST_DESCRIPTOR_CLIENT,
  TEST_DESCRIPTOR_CLIENT_SANDBOXED,
  TEST_REFLECTOR,
  FUZZER_SERVER
};


extern const wchar_t kTestClientChannel[];
extern const wchar_t kReflectorChannel[];
extern const wchar_t kFuzzerChannel[];

class MessageLoopForIO;
namespace IPC {
class Channel;
}  


class IPCChannelTest : public MultiProcessTest {
 protected:

  
  virtual void SetUp();
  virtual void TearDown();

  
  base::ProcessHandle SpawnChild(ChildType child_type,
                                 IPC::Channel *channel);

  
  MessageLoopForIO *message_loop_;
};

#endif  
