








#include <Python.h>

typedef struct kinfo_proc kinfo_proc;

int  psutil_get_proc_list(struct kinfo_proc **procList, size_t *procCount);
char *psutil_get_cmd_args(long pid, size_t *argsize);
char *psutil_get_cmd_path(long pid, size_t *pathsize);
int  psutil_pid_exists(long pid);
PyObject* psutil_get_arg_list(long pid);
