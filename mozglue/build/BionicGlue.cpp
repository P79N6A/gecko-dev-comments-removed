




#include <pthread.h>
#include <string.h>
#include <unistd.h>

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










static pthread_mutex_t  _pr_envLock = PTHREAD_MUTEX_INITIALIZER;

extern "C" NS_EXPORT char*
__wrap_PR_GetEnv(const char *var)
{
    char *ev;

    pthread_mutex_lock(&_pr_envLock);
    ev = getenv(var);
    pthread_mutex_unlock(&_pr_envLock);
    return ev;
}

extern "C" NS_EXPORT int
__wrap_PR_SetEnv(const char *string)
{
    int result;

    if ( !strchr(string, '=')) return(-1);

    pthread_mutex_lock(&_pr_envLock);
    result = putenv(string);
    pthread_mutex_unlock(&_pr_envLock);
    return (result)? -1 : 0;
}

extern "C" NS_EXPORT pthread_mutex_t *
PR_GetEnvLock(void)
{
  return &_pr_envLock;
}
