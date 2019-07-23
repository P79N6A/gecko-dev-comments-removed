



#include "chrome/common/task_queue.h"

#include "base/stl_util-inl.h"

TaskQueue::TaskQueue() {
}

TaskQueue::~TaskQueue() {
  
  STLDeleteElements(&queue_);
}

void TaskQueue::Run() {
  
  if (queue_.empty())
    return;

  std::deque<Task*> ready;
  queue_.swap(ready);

  
  std::deque<Task*>::const_iterator task;
  for (task = ready.begin(); task != ready.end(); ++task) {
    
    (*task)->Run();
    delete (*task);
  }
}

void TaskQueue::Push(Task* task) {
  
  queue_.push_back(task);
}

void TaskQueue::Clear() {
  
  STLDeleteElements(&queue_);
}

bool TaskQueue::Empty() const {
  return queue_.empty();
}
