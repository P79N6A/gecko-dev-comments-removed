




#include <unistd.h>
#include <pthread.h>
#include <vector>

#define NS_EXPORT __attribute__ ((visibility("default")))


struct AtForkFuncs {
  void (*prepare)(void);
  void (*parent)(void);
  void (*child)(void);
};
static std::vector<AtForkFuncs> atfork;

extern "C" NS_EXPORT int
__wrap_pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
  AtForkFuncs funcs;
  funcs.prepare = prepare;
  funcs.parent = parent;
  funcs.child = child;
  atfork.push_back(funcs);
  return 0;
}

extern "C" NS_EXPORT pid_t
__wrap_fork(void)
{
  pid_t pid;
  for (std::vector<AtForkFuncs>::reverse_iterator it = atfork.rbegin();
       it < atfork.rend(); ++it)
    if (it->prepare)
      it->prepare();

  switch ((pid = fork())) {
  case 0:
    for (std::vector<AtForkFuncs>::iterator it = atfork.begin();
         it < atfork.end(); ++it)
      if (it->child)
        it->child();
    break;
  default:
    for (std::vector<AtForkFuncs>::iterator it = atfork.begin();
         it < atfork.end(); ++it)
      if (it->parent)
        it->parent();
  }
  return pid;
}

extern "C" NS_EXPORT int
__wrap_raise(int sig)
{
  return pthread_kill(pthread_self(), sig);
}

