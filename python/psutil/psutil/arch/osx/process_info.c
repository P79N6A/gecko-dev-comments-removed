










#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>  
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysctl.h>
#include <libproc.h>

#include "process_info.h"
#include "../../_psutil_common.h"





int
pid_exists(long pid)
{
    int kill_ret;

    
    if (pid < 0) {
        return 0;
    }

    
    kill_ret = kill(pid , 0);
    if ( (0 == kill_ret) || (EPERM == errno) ) {
        return 1;
    }

    
    return 0;
}











int
get_proc_list(kinfo_proc **procList, size_t *procCount)
{
    

    static const int mib3[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
    size_t           size, size2;
    void            *ptr;
    int              err, lim = 8;  

    assert( procList != NULL);
    assert(*procList == NULL);
    assert(procCount != NULL);

    *procCount = 0;

    










    while (lim-- > 0) {
        size = 0;
        if (sysctl((int *)mib3, 3, NULL, &size, NULL, 0) == -1) {
            return errno;
        }

        size2 = size + (size >> 3);  
        if (size2 > size) {
            ptr = malloc(size2);
            if (ptr == NULL) {
                ptr = malloc(size);
            } else {
                size = size2;
            }
        }
        else {
            ptr = malloc(size);
        }
        if (ptr == NULL) {
            return ENOMEM;
        }

        if (sysctl((int *)mib3, 3, ptr, &size, NULL, 0) == -1) {
            err = errno;
            free(ptr);
            if (err != ENOMEM) {
                return err;
            }

        } else {
            *procList = (kinfo_proc *)ptr;
            *procCount = size / sizeof(kinfo_proc);
            return 0;
        }
    }
    return ENOMEM;
}



int
get_argmax()
{
    int argmax;
    int mib[] = { CTL_KERN, KERN_ARGMAX };
    size_t size = sizeof(argmax);

    if (sysctl(mib, 2, &argmax, &size, NULL, 0) == 0) {
        return argmax;
    }
    return 0;
}



PyObject*
get_arg_list(long pid)
{
    int mib[3];
    int nargs;
    int len;
    char *procargs = NULL;
    char *arg_ptr;
    char *arg_end;
    char *curr_arg;
    size_t argmax;
    PyObject *arg = NULL;
    PyObject *arglist = NULL;

    
    if (pid == 0) {
        return Py_BuildValue("[]");
    }

    
    argmax = get_argmax();
    if (! argmax) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    procargs = (char *)malloc(argmax);
    if (NULL == procargs) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;
    if (sysctl(mib, 3, procargs, &argmax, NULL, 0) < 0) {
        if (EINVAL == errno) { 
            if ( pid_exists(pid) ) {
                AccessDenied();
            } else {
                NoSuchProcess();
            }
        }
        goto error;
    }

    arg_end = &procargs[argmax];
    
    memcpy(&nargs, procargs, sizeof(nargs));

    arg_ptr = procargs + sizeof(nargs);
    len = strlen(arg_ptr);
    arg_ptr += len + 1;

    if (arg_ptr == arg_end) {
        free(procargs);
        return Py_BuildValue("[]");
    }

    
    for (; arg_ptr < arg_end; arg_ptr++) {
        if (*arg_ptr != '\0') {
            break;
        }
    }

    
    curr_arg = arg_ptr;
    arglist = Py_BuildValue("[]");
    if (!arglist)
        goto error;
    while (arg_ptr < arg_end && nargs > 0) {
        if (*arg_ptr++ == '\0') {
            arg = Py_BuildValue("s", curr_arg);
            if (!arg)
                goto error;
            if (PyList_Append(arglist, arg))
                goto error;
            Py_DECREF(arg);
            
            curr_arg = arg_ptr;
            nargs--;
        }
    }

    free(procargs);
    return arglist;

error:
    Py_XDECREF(arg);
    Py_XDECREF(arglist);
    if (procargs != NULL)
        free(procargs);
    return NULL;
}


int
get_kinfo_proc(pid_t pid, struct kinfo_proc *kp)
{
    int mib[4];
    size_t len;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = pid;

    
    len = sizeof(struct kinfo_proc);

    
    if (sysctl(mib, 4, kp, &len, NULL, 0) == -1) {
        
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    


    if (len == 0) {
        NoSuchProcess();
        return -1;
    }
    return 0;
}





int
psutil_proc_pidinfo(long pid, int flavor, void *pti, int size)
{
    int ret = proc_pidinfo((int)pid, flavor, 0, pti, size);
    if (ret == 0) {
        if (! pid_exists(pid)) {
            NoSuchProcess();
            return 0;
        }
        else {
            AccessDenied();
            return 0;
        }
    }
    else if (ret != size) {
        AccessDenied();
        return 0;
    }
    else {
        return 1;
    }
}
