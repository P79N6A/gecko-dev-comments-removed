







#ifndef _GNU_SOURCE
    #define _GNU_SOURCE 1
#endif
#include <Python.h>
#include <errno.h>
#include <stdlib.h>
#include <mntent.h>
#include <features.h>
#include <utmp.h>
#include <sched.h>
#include <linux/version.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

#include "_psutil_linux.h"


static const int NCPUS_START = sizeof(unsigned long) * CHAR_BIT;


#define PSUTIL_HAVE_IOPRIO defined(__NR_ioprio_get) && defined(__NR_ioprio_set)


#define PSUTIL_HAVE_PRLIMIT \
    (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)) && \
    (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 13) && \
    defined(__NR_prlimit64)

#if PSUTIL_HAVE_PRLIMIT
    #define _FILE_OFFSET_BITS 64
    #include <time.h>
    #include <sys/resource.h>
#endif


#if PSUTIL_HAVE_IOPRIO
enum {
    IOPRIO_WHO_PROCESS = 1,
};

static inline int
ioprio_get(int which, int who)
{
    return syscall(__NR_ioprio_get, which, who);
}

static inline int
ioprio_set(int which, int who, int ioprio)
{
    return syscall(__NR_ioprio_set, which, who, ioprio);
}

#define IOPRIO_CLASS_SHIFT 13
#define IOPRIO_PRIO_MASK ((1UL << IOPRIO_CLASS_SHIFT) - 1)

#define IOPRIO_PRIO_CLASS(mask) ((mask) >> IOPRIO_CLASS_SHIFT)
#define IOPRIO_PRIO_DATA(mask) ((mask) & IOPRIO_PRIO_MASK)
#define IOPRIO_PRIO_VALUE(class, data) (((class) << IOPRIO_CLASS_SHIFT) | data)





static PyObject *
psutil_proc_ioprio_get(PyObject *self, PyObject *args)
{
    long pid;
    int ioprio, ioclass, iodata;
    if (! PyArg_ParseTuple(args, "l", &pid)) {
        return NULL;
    }
    ioprio = ioprio_get(IOPRIO_WHO_PROCESS, pid);
    if (ioprio == -1) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }
    ioclass = IOPRIO_PRIO_CLASS(ioprio);
    iodata = IOPRIO_PRIO_DATA(ioprio);
    return Py_BuildValue("ii", ioclass, iodata);
}







static PyObject *
psutil_proc_ioprio_set(PyObject *self, PyObject *args)
{
    long pid;
    int ioprio, ioclass, iodata;
    int retval;

    if (! PyArg_ParseTuple(args, "lii", &pid, &ioclass, &iodata)) {
        return NULL;
    }
    ioprio = IOPRIO_PRIO_VALUE(ioclass, iodata);
    retval = ioprio_set(IOPRIO_WHO_PROCESS, pid, ioprio);
    if (retval == -1) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }
    Py_INCREF(Py_None);
    return Py_None;
}
#endif


#if PSUTIL_HAVE_PRLIMIT





static PyObject *
psutil_linux_prlimit(PyObject *self, PyObject *args)
{
    long pid;
    int ret, resource;
    struct rlimit old, new;
    struct rlimit *newp = NULL;
    PyObject *soft = NULL;
    PyObject *hard = NULL;

    if (! PyArg_ParseTuple(args, "li|OO", &pid, &resource, &soft, &hard)) {
        return NULL;
    }

    
    if (soft == NULL && hard == NULL) {
        ret = prlimit(pid, resource, NULL, &old);
        if (ret == -1)
            return PyErr_SetFromErrno(PyExc_OSError);
#if defined(PSUTIL_HAVE_LONG_LONG)
        if (sizeof(old.rlim_cur) > sizeof(long)) {
            return Py_BuildValue("LL",
                                 (PY_LONG_LONG)old.rlim_cur,
                                 (PY_LONG_LONG)old.rlim_max);
        }
#endif
        return Py_BuildValue("ll", (long)old.rlim_cur, (long)old.rlim_max);
    }

    
    else {
#if defined(PSUTIL_HAVE_LARGEFILE_SUPPORT)
        new.rlim_cur = PyLong_AsLongLong(soft);
        if (new.rlim_cur == (rlim_t) - 1 && PyErr_Occurred())
            return NULL;
        new.rlim_max = PyLong_AsLongLong(hard);
        if (new.rlim_max == (rlim_t) - 1 && PyErr_Occurred())
            return NULL;
#else
        new.rlim_cur = PyLong_AsLong(soft);
        if (new.rlim_cur == (rlim_t) - 1 && PyErr_Occurred())
            return NULL;
        new.rlim_max = PyLong_AsLong(hard);
        if (new.rlim_max == (rlim_t) - 1 && PyErr_Occurred())
            return NULL;
#endif
        newp = &new;
        ret = prlimit(pid, resource, newp, &old);
        if (ret == -1)
            return PyErr_SetFromErrno(PyExc_OSError);
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif






static PyObject *
psutil_disk_partitions(PyObject *self, PyObject *args)
{
    FILE *file = NULL;
    struct mntent *entry;
    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;

    if (py_retlist == NULL)
        return NULL;

    
    Py_BEGIN_ALLOW_THREADS
    file = setmntent(MOUNTED, "r");
    Py_END_ALLOW_THREADS
    if ((file == 0) || (file == NULL)) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, MOUNTED);
        goto error;
    }

    while ((entry = getmntent(file))) {
        if (entry == NULL) {
            PyErr_Format(PyExc_RuntimeError, "getmntent() failed");
            goto error;
        }
        py_tuple = Py_BuildValue("(ssss)",
                                 entry->mnt_fsname,  
                                 entry->mnt_dir,     
                                 entry->mnt_type,    
                                 entry->mnt_opts);   
        if (! py_tuple)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_tuple);
    }
    endmntent(file);
    return py_retlist;

error:
    if (file != NULL)
        endmntent(file);
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    return NULL;
}





static PyObject *
psutil_linux_sysinfo(PyObject *self, PyObject *args)
{
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    
    return Py_BuildValue(
        "(KKKKKK)",
        (unsigned long long)info.totalram  * info.mem_unit,   
        (unsigned long long)info.freeram   * info.mem_unit,   
        (unsigned long long)info.bufferram * info.mem_unit,   
        (unsigned long long)info.sharedram * info.mem_unit,   
        (unsigned long long)info.totalswap * info.mem_unit,   
        (unsigned long long)info.freeswap  * info.mem_unit);  
}








#ifdef CPU_ALLOC

static PyObject *
psutil_proc_cpu_affinity_get(PyObject *self, PyObject *args)
{
    int cpu, ncpus, count, cpucount_s;
    long pid;
    size_t setsize;
    cpu_set_t *mask = NULL;
    PyObject *res = NULL;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    ncpus = NCPUS_START;
    while (1) {
        setsize = CPU_ALLOC_SIZE(ncpus);
        mask = CPU_ALLOC(ncpus);
        if (mask == NULL)
            return PyErr_NoMemory();
        if (sched_getaffinity(pid, setsize, mask) == 0)
            break;
        CPU_FREE(mask);
        if (errno != EINVAL)
            return PyErr_SetFromErrno(PyExc_OSError);
        if (ncpus > INT_MAX / 2) {
            PyErr_SetString(PyExc_OverflowError, "could not allocate "
                            "a large enough CPU set");
            return NULL;
        }
        ncpus = ncpus * 2;
    }

    res = PyList_New(0);
    if (res == NULL)
        goto error;

    cpucount_s = CPU_COUNT_S(setsize, mask);
    for (cpu = 0, count = cpucount_s; count; cpu++) {
        if (CPU_ISSET_S(cpu, setsize, mask)) {
#if PY_MAJOR_VERSION >= 3
            PyObject *cpu_num = PyLong_FromLong(cpu);
#else
            PyObject *cpu_num = PyInt_FromLong(cpu);
#endif
            --count;
            if (cpu_num == NULL)
                goto error;
            if (PyList_Append(res, cpu_num)) {
                Py_DECREF(cpu_num);
                goto error;
            }
            Py_DECREF(cpu_num);
        }
    }
    CPU_FREE(mask);
    return res;

error:
    if (mask)
        CPU_FREE(mask);
    Py_XDECREF(res);
    return NULL;
}
#else


static PyObject *
psutil_proc_cpu_affinity_get(PyObject *self, PyObject *args)
{
    cpu_set_t cpuset;
    unsigned int len = sizeof(cpu_set_t);
    long pid;
	int i;
	PyObject* ret_list;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

	CPU_ZERO(&cpuset);
    if (sched_getaffinity(pid, len, &cpuset) < 0) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    ret_list = PyList_New(0);

    for (i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &cpuset)) {
            PyList_Append(ret_list, Py_BuildValue("i", i));
        }
    }

    return ret_list;
}
#endif




static PyObject *
psutil_proc_cpu_affinity_set(PyObject *self, PyObject *args)
{
    cpu_set_t cpu_set;
    size_t len;
    long pid;
    int i, seq_len;
    PyObject *py_cpu_set;
    PyObject *py_cpu_seq = NULL;

    if (!PyArg_ParseTuple(args, "lO", &pid, &py_cpu_set)) {
        goto error;
    }

    if (!PySequence_Check(py_cpu_set)) {
        
        
        
        PyErr_Format(PyExc_TypeError, "sequence argument expected");
        goto error;
    }

    py_cpu_seq = PySequence_Fast(py_cpu_set, "expected a sequence or integer");
    if (!py_cpu_seq) {
        goto error;
    }
    seq_len = PySequence_Fast_GET_SIZE(py_cpu_seq);
    CPU_ZERO(&cpu_set);
    for (i = 0; i < seq_len; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(py_cpu_seq, i);
#if PY_MAJOR_VERSION >= 3
        long value = PyLong_AsLong(item);
#else
        long value = PyInt_AsLong(item);
#endif
        if (value == -1 && PyErr_Occurred()) {
            goto error;
        }
        CPU_SET(value, &cpu_set);
    }

    len = sizeof(cpu_set);
    if (sched_setaffinity(pid, len, &cpu_set)) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    Py_DECREF(py_cpu_seq);
    Py_INCREF(Py_None);
    return Py_None;

error:
    if (py_cpu_seq != NULL) {
        Py_DECREF(py_cpu_seq);
	}

    return NULL;
}





static PyObject *
psutil_users(PyObject *self, PyObject *args)
{
    PyObject *ret_list = PyList_New(0);
    PyObject *tuple = NULL;
    PyObject *user_proc = NULL;
    struct utmp *ut;

    if (ret_list == NULL)
        return NULL;
    setutent();
    while (NULL != (ut = getutent())) {
        tuple = NULL;
        user_proc = NULL;
        if (ut->ut_type == USER_PROCESS)
            user_proc = Py_True;
        else
            user_proc = Py_False;
        tuple = Py_BuildValue(
            "(sssfO)",
            ut->ut_user,              
            ut->ut_line,              
            ut->ut_host,              
            (float)ut->ut_tv.tv_sec,  
            user_proc                 
        );
    if (! tuple)
            goto error;
        if (PyList_Append(ret_list, tuple))
            goto error;
        Py_DECREF(tuple);
    }
    endutent();
    return ret_list;

error:
    Py_XDECREF(tuple);
    Py_XDECREF(user_proc);
    Py_DECREF(ret_list);
    endutent();
    return NULL;
}





static PyMethodDef
PsutilMethods[] =
{
    

#if PSUTIL_HAVE_IOPRIO
    {"proc_ioprio_get", psutil_proc_ioprio_get, METH_VARARGS,
     "Get process I/O priority"},
    {"proc_ioprio_set", psutil_proc_ioprio_set, METH_VARARGS,
     "Set process I/O priority"},
#endif
    {"proc_cpu_affinity_get", psutil_proc_cpu_affinity_get, METH_VARARGS,
     "Return process CPU affinity as a Python long (the bitmask)."},
    {"proc_cpu_affinity_set", psutil_proc_cpu_affinity_set, METH_VARARGS,
     "Set process CPU affinity; expects a bitmask."},

    

    {"disk_partitions", psutil_disk_partitions, METH_VARARGS,
     "Return disk mounted partitions as a list of tuples including "
     "device, mount point and filesystem type"},
    {"users", psutil_users, METH_VARARGS,
     "Return currently connected users as a list of tuples"},

    

    {"linux_sysinfo", psutil_linux_sysinfo, METH_VARARGS,
     "A wrapper around sysinfo(), return system memory usage statistics"},
#if PSUTIL_HAVE_PRLIMIT
    {"linux_prlimit", psutil_linux_prlimit, METH_VARARGS,
     "Get or set process resource limits."},
#endif


    {NULL, NULL, 0, NULL}
};

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
#endif

#if PY_MAJOR_VERSION >= 3

static int
psutil_linux_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int
psutil_linux_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

static struct PyModuleDef
        moduledef = {
    PyModuleDef_HEAD_INIT,
    "psutil_linux",
    NULL,
    sizeof(struct module_state),
    PsutilMethods,
    NULL,
    psutil_linux_traverse,
    psutil_linux_clear,
    NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC PyInit__psutil_linux(void)

#else
#define INITERROR return

void init_psutil_linux(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("_psutil_linux", PsutilMethods);
#endif


#if PSUTIL_HAVE_PRLIMIT
    PyModule_AddIntConstant(module, "RLIM_INFINITY", RLIM_INFINITY);
    PyModule_AddIntConstant(module, "RLIMIT_AS", RLIMIT_AS);
    PyModule_AddIntConstant(module, "RLIMIT_CORE", RLIMIT_CORE);
    PyModule_AddIntConstant(module, "RLIMIT_CPU", RLIMIT_CPU);
    PyModule_AddIntConstant(module, "RLIMIT_DATA", RLIMIT_DATA);
    PyModule_AddIntConstant(module, "RLIMIT_FSIZE", RLIMIT_FSIZE);
    PyModule_AddIntConstant(module, "RLIMIT_LOCKS", RLIMIT_LOCKS);
    PyModule_AddIntConstant(module, "RLIMIT_MEMLOCK", RLIMIT_MEMLOCK);
    PyModule_AddIntConstant(module, "RLIMIT_NOFILE", RLIMIT_NOFILE);
    PyModule_AddIntConstant(module, "RLIMIT_NPROC", RLIMIT_NPROC);
    PyModule_AddIntConstant(module, "RLIMIT_RSS", RLIMIT_RSS);
    PyModule_AddIntConstant(module, "RLIMIT_STACK", RLIMIT_STACK);
#ifdef RLIMIT_MSGQUEUE
    PyModule_AddIntConstant(module, "RLIMIT_MSGQUEUE", RLIMIT_MSGQUEUE);
#endif
#ifdef RLIMIT_NICE
    PyModule_AddIntConstant(module, "RLIMIT_NICE", RLIMIT_NICE);
#endif
#ifdef RLIMIT_RTPRIO
    PyModule_AddIntConstant(module, "RLIMIT_RTPRIO", RLIMIT_RTPRIO);
#endif
#ifdef RLIMIT_RTTIME
    PyModule_AddIntConstant(module, "RLIMIT_RTTIME", RLIMIT_RTTIME);
#endif
#ifdef RLIMIT_SIGPENDING
    PyModule_AddIntConstant(module, "RLIMIT_SIGPENDING", RLIMIT_SIGPENDING);
#endif
#endif

    if (module == NULL) {
        INITERROR;
    }
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
