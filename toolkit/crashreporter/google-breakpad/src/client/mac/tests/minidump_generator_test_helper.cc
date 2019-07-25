
































#include <unistd.h>

#include "client/mac/handler/exception_handler.h"
#include "common/mac/MachIPC.h"

using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::ReceivePort;

int main(int argc, char** argv) {
  if (argc < 2)
    return 1;

  if (strcmp(argv[1], "crash") != 0) {
    const int kTimeoutMs = 2000;
    
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());
    child_message.AddDescriptor(mach_thread_self());

    MachPortSender child_sender(argv[1]);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS) {
      fprintf(stderr, "Error sending message from child process!\n");
      exit(1);
    }

    
    while (true) {
      sleep(100);
    }
  } else if (argc == 3 && strcmp(argv[1], "crash") == 0) {
    
    google_breakpad::ExceptionHandler eh("", NULL, NULL, NULL, true, argv[2]);
    
    int *a = (int*)0x42;
    *a = 1;
  }

  return 0;
}
