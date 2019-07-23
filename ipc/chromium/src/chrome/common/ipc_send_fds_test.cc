



#include "build/build_config.h"

#include "chrome/common/ipc_tests.h"

#if defined(OS_MACOSX)
extern "C" {
#include <sandbox.h>
}
#endif
#include <fcntl.h>
#include <sys/stat.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"
#include "chrome/common/ipc_message_utils.h"

#if defined(OS_POSIX)

namespace {

const unsigned kNumFDsToSend = 20;
const char* kDevZeroPath = "/dev/zero";

static void VerifyAndCloseDescriptor(int fd, ino_t inode_num) {
  
  char buf;
  ssize_t amt_read = read(fd, &buf, 1);
  ASSERT_EQ(amt_read, 1);
  ASSERT_EQ(buf, 0);  

  struct stat st;
  ASSERT_EQ(fstat(fd, &st), 0);

  ASSERT_EQ(close(fd), 0);

  
  
  ASSERT_EQ(inode_num, st.st_ino);
}

class MyChannelDescriptorListener : public IPC::Channel::Listener {
 public:
  MyChannelDescriptorListener(ino_t expected_inode_num)
      : expected_inode_num_(expected_inode_num),
        num_fds_received_(0) {}

  virtual void OnMessageReceived(const IPC::Message& message) {
    void* iter = NULL;

    ++num_fds_received_;
    base::FileDescriptor descriptor;

    ASSERT_TRUE(
        IPC::ParamTraits<base::FileDescriptor>::Read(
            &message, &iter, &descriptor));

    VerifyAndCloseDescriptor(descriptor.fd, expected_inode_num_);
    if (num_fds_received_ == kNumFDsToSend) {
      MessageLoop::current()->Quit();
    }
  }

  virtual void OnChannelError() {
    MessageLoop::current()->Quit();
  }
 private:
  ino_t expected_inode_num_;
  unsigned num_fds_received_;
};

void TestDescriptorServer(IPC::Channel &chan,
                          base::ProcessHandle process_handle) {
  ASSERT_TRUE(process_handle);

  for (unsigned i = 0; i < kNumFDsToSend; ++i) {
    base::FileDescriptor descriptor;
    const int fd = open(kDevZeroPath, O_RDONLY);
    ASSERT_GE(fd, 0);
    descriptor.auto_close = true;
    descriptor.fd = fd;

    IPC::Message* message = new IPC::Message(0, 
                                             3, 
                                             IPC::Message::PRIORITY_NORMAL);
    IPC::ParamTraits<base::FileDescriptor>::Write(message, descriptor);
    chan.Send(message);
  }

  
  MessageLoop::current()->Run();

  
  chan.Close();

  
  EXPECT_TRUE(base::WaitForSingleProcess(process_handle, 5000));
}

int TestDescriptorClient(ino_t expected_inode_num) {
  MessageLoopForIO main_message_loop;
  MyChannelDescriptorListener listener(expected_inode_num);

  
  IPC::Channel chan(kTestClientChannel, IPC::Channel::MODE_CLIENT,
                    &listener);
  chan.Connect();
  MessageLoop::current()->Run();

  return 0;
}

}  


#if defined(OS_MACOSX)

MULTIPROCESS_TEST_MAIN(RunTestDescriptorClientSandboxed) {
  struct stat st;
  const int fd = open(kDevZeroPath, O_RDONLY);
  fstat(fd, &st);
  HANDLE_EINTR(close(fd));

  
  char* error_buff = NULL;
  int error = sandbox_init(kSBXProfilePureComputation, SANDBOX_NAMED,
                           &error_buff);
  bool success = (error == 0 && error_buff == NULL);
  if (!success) {
    return -1;
  }

  sandbox_free_error(error_buff);

  
  if (open(kDevZeroPath, O_RDONLY) != -1) {
    LOG(ERROR) << "Sandbox wasn't properly enabled";
    return -1;
  }

  
  return TestDescriptorClient(st.st_ino);
}


TEST_F(IPCChannelTest, DescriptorTestSandboxed) {
    
  MyChannelDescriptorListener listener(-1);

  IPC::Channel chan(kTestClientChannel, IPC::Channel::MODE_SERVER,
                    &listener);
  chan.Connect();

  base::ProcessHandle process_handle = SpawnChild(
      TEST_DESCRIPTOR_CLIENT_SANDBOXED,
      &chan);
  TestDescriptorServer(chan, process_handle);
}
#endif  

MULTIPROCESS_TEST_MAIN(RunTestDescriptorClient) {
  struct stat st;
  const int fd = open(kDevZeroPath, O_RDONLY);
  fstat(fd, &st);
  HANDLE_EINTR(close(fd));

  return TestDescriptorClient(st.st_ino);
}

TEST_F(IPCChannelTest, DescriptorTest) {
    
  MyChannelDescriptorListener listener(-1);

  IPC::Channel chan(kTestClientChannel, IPC::Channel::MODE_SERVER,
                    &listener);
  chan.Connect();

  base::ProcessHandle process_handle = SpawnChild(TEST_DESCRIPTOR_CLIENT,
                                                  &chan);
  TestDescriptorServer(chan, process_handle);
}

#endif  
