

















#ifndef gmp_task_utils_h_
#define gmp_task_utils_h_

#include "gmp-platform.h"

class gmp_task_args_base : public GMPTask {
public:
  virtual void Destroy() { delete this; }
  virtual void Run() = 0;
};
















#include "gmp-task-utils-generated.h"

#endif 
