



#ifndef CHROME_COMMON_IPC_TEST_SINK_H_
#define CHROME_COMMON_IPC_TEST_SINK_H_

#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "chrome/common/ipc_message.h"

namespace IPC {




























class TestSink {
 public:
  TestSink();
  ~TestSink();

  
  
  void OnMessageReceived(const Message& msg);

  
  size_t message_count() const { return messages_.size(); }

  
  void ClearMessages();

  
  
  
  const Message* GetMessageAt(size_t index) const;

  
  
  
  const Message* GetFirstMessageMatching(uint16 id) const;

  
  
  
  
  
  const Message* GetUniqueMessageMatching(uint16 id) const;

 private:
  
  std::vector<Message> messages_;

  DISALLOW_COPY_AND_ASSIGN(TestSink);
};

}  

#endif  
