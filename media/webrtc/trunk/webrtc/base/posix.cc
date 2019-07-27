









#include "webrtc/base/posix.h"

#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
#include "webrtc/base/linuxfdwalk.h"
#endif
#include "webrtc/base/logging.h"

namespace rtc {

#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
static void closefds(void *close_errors, int fd) {
  if (fd <= 2) {
    
    return;
  }
  if (close(fd) < 0) {
    *static_cast<bool *>(close_errors) = true;
  }
}
#endif

enum {
  EXIT_FLAG_CHDIR_ERRORS       = 1 << 0,
#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
  EXIT_FLAG_FDWALK_ERRORS      = 1 << 1,
  EXIT_FLAG_CLOSE_ERRORS       = 1 << 2,
#endif
  EXIT_FLAG_SECOND_FORK_FAILED = 1 << 3,
};

bool RunAsDaemon(const char *file, const char *const argv[]) {
  
  pid_t pid = fork();
  if (pid < 0) {
    LOG_ERR(LS_ERROR) << "fork()";
    return false;
  } else if (!pid) {
    

    
    
    int exit_code = 0;
    if (chdir("/") < 0) {
      exit_code |= EXIT_FLAG_CHDIR_ERRORS;
    }
#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
    bool close_errors = false;
    if (fdwalk(&closefds, &close_errors) < 0) {
      exit_code |= EXIT_FLAG_FDWALK_ERRORS;
    }
    if (close_errors) {
      exit_code |= EXIT_FLAG_CLOSE_ERRORS;
    }
#endif

    
    pid = fork();
    
    
    
    if (pid < 0) {
      exit_code |= EXIT_FLAG_SECOND_FORK_FAILED;
      _exit(exit_code);  
    } else if (!pid) {
      
      
      
      
      
      execvp(file, const_cast<char *const *>(argv));
      _exit(255);  
    }

    
    
    
    _exit(exit_code);
  }

  
  int status;
  pid_t child = waitpid(pid, &status, 0);
  if (child < 0) {
    LOG_ERR(LS_ERROR) << "Error in waitpid()";
    return false;
  }
  if (child != pid) {
    
    LOG(LS_ERROR) << "waitpid() chose wrong child???";
    return false;
  }
  if (!WIFEXITED(status)) {
    LOG(LS_ERROR) << "Intermediate child killed uncleanly";  
    return false;
  }

  int exit_code = WEXITSTATUS(status);
  if (exit_code & EXIT_FLAG_CHDIR_ERRORS) {
    LOG(LS_WARNING) << "Child reported probles calling chdir()";
  }
#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
  if (exit_code & EXIT_FLAG_FDWALK_ERRORS) {
    LOG(LS_WARNING) << "Child reported problems calling fdwalk()";
  }
  if (exit_code & EXIT_FLAG_CLOSE_ERRORS) {
    LOG(LS_WARNING) << "Child reported problems calling close()";
  }
#endif
  if (exit_code & EXIT_FLAG_SECOND_FORK_FAILED) {
    LOG(LS_ERROR) << "Failed to daemonize";
    
    return false;
  }
  return true;
}

}  
