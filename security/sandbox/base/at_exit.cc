



#include "base/at_exit.h"

#include <stddef.h>
#include <ostream>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"

namespace base {







static AtExitManager* g_top_manager = NULL;

AtExitManager::AtExitManager() : next_manager_(g_top_manager) {


#if !defined(COMPONENT_BUILD)
  DCHECK(!g_top_manager);
#endif
  g_top_manager = this;
}

AtExitManager::~AtExitManager() {
  if (!g_top_manager) {
    NOTREACHED() << "Tried to ~AtExitManager without an AtExitManager";
    return;
  }
  DCHECK_EQ(this, g_top_manager);

  ProcessCallbacksNow();
  g_top_manager = next_manager_;
}


void AtExitManager::RegisterCallback(AtExitCallbackType func, void* param) {
  DCHECK(func);
  RegisterTask(base::Bind(func, param));
}


void AtExitManager::RegisterTask(base::Closure task) {
  if (!g_top_manager) {
    NOTREACHED() << "Tried to RegisterCallback without an AtExitManager";
    return;
  }

  AutoLock lock(g_top_manager->lock_);
  g_top_manager->stack_.push(task);
}


void AtExitManager::ProcessCallbacksNow() {
  if (!g_top_manager) {
    NOTREACHED() << "Tried to ProcessCallbacksNow without an AtExitManager";
    return;
  }

  AutoLock lock(g_top_manager->lock_);

  while (!g_top_manager->stack_.empty()) {
    base::Closure task = g_top_manager->stack_.top();
    task.Run();
    g_top_manager->stack_.pop();
  }
}

AtExitManager::AtExitManager(bool shadow) : next_manager_(g_top_manager) {
  DCHECK(shadow || !g_top_manager);
  g_top_manager = this;
}

}  
