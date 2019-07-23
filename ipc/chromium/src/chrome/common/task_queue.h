



#ifndef CHROME_COMMON_TASK_QUEUE_H__
#define CHROME_COMMON_TASK_QUEUE_H__

#include <deque>

#include "base/task.h"




class TaskQueue : public Task {
 public:
  TaskQueue();
  ~TaskQueue();

  
  
  virtual void Run();

  
  
  
  
  
  void Push(Task* task);

  
  void Clear();

  
  bool Empty() const;

 private:
   
   std::deque<Task*> queue_;
};

#endif  
