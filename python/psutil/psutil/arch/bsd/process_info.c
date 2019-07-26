










#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <signal.h>

#include "process_info.h"










int
get_proc_list(struct kinfo_proc **procList, size_t *procCount)
{
    int err;
    struct kinfo_proc * result;
    int done;
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0 };
    
    
    size_t              length;

    assert( procList != NULL);
    assert(*procList == NULL);
    assert(procCount != NULL);

    *procCount = 0;

    










    result = NULL;
    done = 0;
    do {
        assert(result == NULL);
        
        length = 0;
        err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1,
                     NULL, &length, NULL, 0);
        if (err == -1)
            err = errno;

        
        
        if (err == 0) {
            result = malloc(length);
            if (result == NULL)
                err = ENOMEM;
        }

        
        
        if (err == 0) {
            err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1,
                          result, &length, NULL, 0);
            if (err == -1)
                err = errno;
            if (err == 0) {
                done = 1;
            }
            else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    
    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }

    *procList = result;
    *procCount = length / sizeof(struct kinfo_proc);

    assert((err == 0) == (*procList != NULL));
    return err;
}


char
*getcmdpath(long pid, size_t *pathsize)
{
    int  mib[4];
    char *path;
    size_t size = 0;

    


    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = pid;

    
    if (sysctl(mib, 4, NULL, &size, NULL, 0) == -1) {
        return NULL;
    }

    path = malloc(size);
    if (path == NULL) {
        PyErr_SetString(PyExc_MemoryError, "couldn't allocate memory");
        return NULL;
    }

    *pathsize = size;
    if (sysctl(mib, 4, path, &size, NULL, 0) == -1) {
        free(path);
        return NULL;       
    }

    return path;
}














char
*getcmdargs(long pid, size_t *argsize)
{
    int mib[4];
    size_t size, argmax;
    char *procargs = NULL;

    
    mib[0] = CTL_KERN;
    mib[1] = KERN_ARGMAX;

    size = sizeof(argmax);
    if (sysctl(mib, 2, &argmax, &size, NULL, 0) == -1)
        return NULL;

    
    procargs = (char *)malloc(argmax);
    if (procargs == NULL) {
        PyErr_SetString(PyExc_MemoryError, "couldn't allocate memory");
        return NULL;
    }

    


    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_ARGS;
    mib[3] = pid;

    size = argmax;
    if (sysctl(mib, 4, procargs, &size, NULL, 0) == -1) {
        free(procargs);
        return NULL;       
    }

    
    *argsize = size;
    return procargs;
}



PyObject*
get_arg_list(long pid)
{
    char *argstr = NULL;
    int pos = 0;
    size_t argsize = 0;
    PyObject *retlist = Py_BuildValue("[]");
    PyObject *item = NULL;

    if (pid < 0) {
        return retlist;
    }

    argstr = getcmdargs(pid, &argsize);
    if (argstr == NULL) {
        goto error;
    }

    
    
    
    if (argsize > 0) {
        while(pos < argsize) {
            item = Py_BuildValue("s", &argstr[pos]);
            if (!item)
                goto error;
            if (PyList_Append(retlist, item))
                goto error;
            Py_DECREF(item);
            pos = pos + strlen(&argstr[pos]) + 1;
        }
    }

    free(argstr);
    return retlist;

error:
    Py_XDECREF(item);
    Py_DECREF(retlist);
    if (argstr != NULL)
        free(argstr);
    return NULL;
}





int
pid_exists(long pid)
{
    int kill_ret;
    if (pid < 0) {
        return 0;
    }

    
    kill_ret = kill(pid , 0);
    if ((0 == kill_ret) || (EPERM == errno)) {
        return 1;
    }

    
    return 0;
}
