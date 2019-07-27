




























#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>

#include "client/linux/crash_generation/crash_generation_client.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/ignore_ret.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

bool
CrashGenerationClient::RequestDump(const void* blob, size_t blob_size)
{
  int fds[2];
  if (sys_pipe(fds) != 0)
    return false;
  static const unsigned kControlMsgSize = CMSG_SPACE(sizeof(int));

  struct kernel_msghdr msg;
  my_memset(&msg, 0, sizeof(struct kernel_msghdr));
  struct kernel_iovec iov[1];
  iov[0].iov_base = const_cast<void*>(blob);
  iov[0].iov_len = blob_size;

  msg.msg_iov = iov;
  msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
  char cmsg[kControlMsgSize];
  my_memset(cmsg, 0, kControlMsgSize);
  msg.msg_control = cmsg;
  msg.msg_controllen = sizeof(cmsg);

  struct cmsghdr* hdr = CMSG_FIRSTHDR(&msg);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_RIGHTS;
  hdr->cmsg_len = CMSG_LEN(sizeof(int));
  int* p = reinterpret_cast<int*>(CMSG_DATA(hdr));
  *p = fds[1];

  ssize_t ret = HANDLE_EINTR(sys_sendmsg(server_fd_, &msg, 0));
  sys_close(fds[1]);
  if (ret <= 0)
    return false;

  
  char b;
  IGNORE_RET(HANDLE_EINTR(sys_read(fds[0], &b, 1)));

  return true;
}


CrashGenerationClient*
CrashGenerationClient::TryCreate(int server_fd)
{
  if (0 > server_fd)
    return NULL;
  return new CrashGenerationClient(server_fd);
}

}
