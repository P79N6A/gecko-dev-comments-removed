










#include <Python.h>




#undef _FILE_OFFSET_BITS
#define _STRUCTURED_PROC 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/swap.h>
#include <sys/sysinfo.h>
#include <sys/mntent.h>  
#include <sys/mnttab.h>
#include <sys/procfs.h>
#include <fcntl.h>
#include <utmpx.h>
#include <kstat.h>
#include <sys/ioctl.h>
#include <sys/tihdr.h>
#include <stropts.h>
#include <inet/tcp.h>
#include <arpa/inet.h>

#include "_psutil_sunos.h"


#define TV2DOUBLE(t)   (((t).tv_nsec * 0.000000001) + (t).tv_sec)




int
psutil_file_to_struct(char *path, void *fstruct, size_t size)
{
    int fd;
    size_t nbytes;
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, path);
        return 0;
    }
    nbytes = read(fd, fstruct, size);
    if (nbytes <= 0) {
        close(fd);
        PyErr_SetFromErrno(PyExc_OSError);
        return 0;
    }
    if (nbytes != size) {
        close(fd);
        PyErr_SetString(PyExc_RuntimeError, "structure size mismatch");
        return 0;
    }
    close(fd);
    return nbytes;
}






static PyObject*
get_process_basic_info(PyObject* self, PyObject* args)
{
    int pid;
    char path[100];
    psinfo_t info;

    if (! PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    sprintf(path, "/proc/%i/psinfo", pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    return Py_BuildValue("ikkdiiik",
                         info.pr_ppid,              
                         info.pr_rssize,            
                         info.pr_size,              
                         TV2DOUBLE(info.pr_start),  
                         info.pr_lwp.pr_nice,       
                         info.pr_nlwp,              
                         info.pr_lwp.pr_state,      
                         info.pr_ttydev             
                         );
}





static PyObject*
get_process_name_and_args(PyObject* self, PyObject* args)
{
    int pid;
    char path[100];
    psinfo_t info;

    if (! PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    sprintf(path, "/proc/%i/psinfo", pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    return Py_BuildValue("ss", info.pr_fname,
                               info.pr_psargs);
}





static PyObject*
get_process_cpu_times(PyObject* self, PyObject* args)
{
    int pid;
    char path[100];
    pstatus_t info;

    if (! PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    sprintf(path, "/proc/%i/status", pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    
    return Py_BuildValue("dd", TV2DOUBLE(info.pr_utime),
                               TV2DOUBLE(info.pr_stime));
}





static PyObject*
get_process_cred(PyObject* self, PyObject* args)
{
    int pid;
    char path[100];
    prcred_t info;

    if (! PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    sprintf(path, "/proc/%i/cred", pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    return Py_BuildValue("iiiiii", info.pr_ruid, info.pr_euid, info.pr_suid,
                                   info.pr_rgid, info.pr_egid, info.pr_sgid);
}





static PyObject*
get_process_num_ctx_switches(PyObject* self, PyObject* args)
{
    int pid;
    char path[100];
    prusage_t info;

    if (! PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    sprintf(path, "/proc/%i/usage", pid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    return Py_BuildValue("kk", info.pr_vctx, info.pr_ictx);
}












































static PyObject*
query_process_thread(PyObject* self, PyObject* args)
{
    int pid, tid;
    char path[100];
    lwpstatus_t info;

    if (! PyArg_ParseTuple(args, "ii", &pid, &tid))
        return NULL;
    sprintf(path, "/proc/%i/lwp/%i/lwpstatus", pid, tid);
    if (! psutil_file_to_struct(path, (void *)&info, sizeof(info)))
        return NULL;
    return Py_BuildValue("dd", TV2DOUBLE(info.pr_utime),
                               TV2DOUBLE(info.pr_stime));
}





static PyObject*
get_swap_mem(PyObject* self, PyObject* args)
{























































    kstat_ctl_t    *kc;
    kstat_t        *k;
    cpu_stat_t    *cpu;
    int            cpu_count = 0;
    int         flag = 0;
    uint_t      sin = 0;
    uint_t      sout = 0;

    kc = kstat_open();
    if (kc == NULL) {
        return PyErr_SetFromErrno(PyExc_OSError);;
    }

    k = kc->kc_chain;
      while (k != NULL) {
        if((strncmp(k->ks_name, "cpu_stat", 8) == 0) && \
            (kstat_read(kc, k, NULL) != -1) )
        {
            flag = 1;
            cpu = (cpu_stat_t*) k->ks_data;
            sin += cpu->cpu_vminfo.pgswapin;    
            sout += cpu->cpu_vminfo.pgswapout;  
        }
        cpu_count += 1;
        k = k->ks_next;
    }
    kstat_close(kc);
    if (!flag) {
        PyErr_SetString(PyExc_RuntimeError, "no swap device was found");
        return NULL;
    }
    return Py_BuildValue("(II)", sin, sout);
}





static PyObject*
get_system_users(PyObject* self, PyObject* args)
{
    struct utmpx *ut;
    PyObject *ret_list = PyList_New(0);
    PyObject *tuple = NULL;
    PyObject *user_proc = NULL;

    if (ret_list == NULL)
        return NULL;

    while (NULL != (ut = getutxent())) {
        if (ut->ut_type == USER_PROCESS)
            user_proc = Py_True;
        else
            user_proc = Py_False;
        tuple = Py_BuildValue("(sssfO)",
            ut->ut_user,              
            ut->ut_line,              
            ut->ut_host,              
            (float)ut->ut_tv.tv_sec,  
            user_proc                 
        );
        if (tuple == NULL)
            goto error;
        if (PyList_Append(ret_list, tuple))
            goto error;
        Py_DECREF(tuple);
    }
    endutent();

    return ret_list;

error:
    Py_XDECREF(tuple);
    Py_DECREF(ret_list);
    if (ut != NULL)
        endutent();
    return NULL;
}






static PyObject*
get_disk_partitions(PyObject* self, PyObject* args)
{
    FILE *file;
    struct mnttab mt;
    PyObject* py_retlist = PyList_New(0);
    PyObject* py_tuple = NULL;

    if (py_retlist == NULL)
        return NULL;

    file = fopen(MNTTAB, "rb");
    if (file == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    while (getmntent(file, &mt) == 0) {
        py_tuple = Py_BuildValue("(ssss)", mt.mnt_special,  
                                           mt.mnt_mountp,     
                                           mt.mnt_fstype,    
                                           mt.mnt_mntopts);   
        if (py_tuple == NULL)
            goto error;
        if (PyList_Append(py_retlist, py_tuple))
            goto error;
        Py_DECREF(py_tuple);

    }
    fclose(file);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_DECREF(py_retlist);
    if (file != NULL)
        fclose(file);
    return NULL;
}





static PyObject*
get_system_per_cpu_times(PyObject* self, PyObject* args)
{
    kstat_ctl_t *kc;
    kstat_t *ksp;
    cpu_stat_t cs;
    int numcpus;
    int i;
    PyObject* py_retlist = PyList_New(0);
    PyObject* py_cputime = NULL;

    if (py_retlist == NULL)
        return NULL;

    kc = kstat_open();
    if (kc == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    numcpus = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    for (i=0; i<=numcpus; i++) {
        ksp = kstat_lookup(kc, "cpu_stat", i, NULL);
        if (ksp == NULL) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto error;
        }
        if (kstat_read(kc, ksp, &cs) == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto error;
        }

        py_cputime = Py_BuildValue("ffff",
                                   (float)cs.cpu_sysinfo.cpu[CPU_USER],
                                   (float)cs.cpu_sysinfo.cpu[CPU_KERNEL],
                                   (float)cs.cpu_sysinfo.cpu[CPU_IDLE],
                                   (float)cs.cpu_sysinfo.cpu[CPU_WAIT]);
        if (py_cputime == NULL)
            goto error;
        if (PyList_Append(py_retlist, py_cputime))
            goto error;
        Py_DECREF(py_cputime);

    }

    kstat_close(kc);
    return py_retlist;

error:
    Py_XDECREF(py_cputime);
    Py_DECREF(py_retlist);
    if (kc != NULL)
        kstat_close(kc);
    return NULL;
}





static PyObject*
get_disk_io_counters(PyObject* self, PyObject* args)
{
    kstat_ctl_t *kc;
    kstat_t *ksp;
    kstat_io_t kio;
    PyObject* py_retdict = PyDict_New();
    PyObject* py_disk_info = NULL;

    if (py_retdict == NULL)
        return NULL;
    kc = kstat_open();
    if (kc == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);;
        goto error;
    }
    ksp = kc->kc_chain;
    while (ksp != NULL) {
        if (ksp->ks_type == KSTAT_TYPE_IO) {
            if (strcmp(ksp->ks_class, "disk") == 0) {
                if (kstat_read(kc, ksp, &kio) == -1) {
                    kstat_close(kc);
                    return PyErr_SetFromErrno(PyExc_OSError);;
                }
                py_disk_info = Py_BuildValue("(IIKKLL)",
                     kio.reads,
                     kio.writes,
                     kio.nread,
                     kio.nwritten,
                     kio.rtime / 1000 / 1000,  
                     kio.wtime / 1000 / 1000   
                );

                if (!py_disk_info)
                    goto error;
                if (PyDict_SetItemString(py_retdict, ksp->ks_name, py_disk_info))
                    goto error;
                Py_DECREF(py_disk_info);
            }
        }
        ksp = ksp->ks_next;
    }
    kstat_close(kc);

    return py_retdict;

error:
    Py_XDECREF(py_disk_info);
    Py_DECREF(py_retdict);
    if (kc != NULL)
        kstat_close(kc);
    return NULL;
}





static PyObject*
get_process_memory_maps(PyObject* self, PyObject* args)
{
    int pid;
    int fd = -1;
    char path[100];
    char perms[10];
    char *name;
    struct stat st;
    pstatus_t status;

    prxmap_t *xmap = NULL, *p;
    off_t size;
    size_t nread;
    int nmap;
    uintptr_t pr_addr_sz;
    uintptr_t stk_base_sz, brk_base_sz;

    PyObject* pytuple = NULL;
    PyObject* py_retlist = PyList_New(0);

    if (py_retlist == NULL) {
        return NULL;
    }
    if (! PyArg_ParseTuple(args, "i", &pid)) {
        goto error;
    }

    sprintf(path, "/proc/%i/status", pid);
    if (! psutil_file_to_struct(path, (void *)&status, sizeof(status))) {
        goto error;
    }

    sprintf(path, "/proc/%i/xmap", pid);
    if (stat(path, &st) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    size = st.st_size;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    xmap = (prxmap_t *)malloc(size);
    if (xmap == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    nread = pread(fd, xmap, size, 0);
    nmap = nread / sizeof(prxmap_t);
    p = xmap;

    while (nmap) {
        nmap -= 1;
        if (p == NULL) {
            p += 1;
            continue;
        }

        perms[0] = '\0';
        pr_addr_sz = p->pr_vaddr + p->pr_size;

        
        sprintf(perms, "%c%c%c%c%c%c", p->pr_mflags & MA_READ ? 'r' : '-',
                                       p->pr_mflags & MA_WRITE ? 'w' : '-',
                                       p->pr_mflags & MA_EXEC ? 'x' : '-',
                                       p->pr_mflags & MA_SHARED ? 's' : '-',
                                       p->pr_mflags & MA_NORESERVE ? 'R' : '-',
                                       p->pr_mflags & MA_RESERVED1 ? '*' : ' ');

        
        if (strlen(p->pr_mapname) > 0) {
            name = p->pr_mapname;
        }
        else {
            if ((p->pr_mflags & MA_ISM) || (p->pr_mflags & MA_SHM)) {
                name = "[shmid]";
            }
            else {
                stk_base_sz = status.pr_stkbase + status.pr_stksize;
                brk_base_sz = status.pr_brkbase + status.pr_brksize;

                if ((pr_addr_sz > status.pr_stkbase) && (p->pr_vaddr < stk_base_sz)) {
                    name = "[stack]";
                }
                else if ((p->pr_mflags & MA_ANON) && \
                         (pr_addr_sz > status.pr_brkbase) && \
                         (p->pr_vaddr < brk_base_sz)) {
                    name = "[heap]";
                }
                else {
                    name = "[anon]";
                }
            }
        }

        pytuple = Py_BuildValue("iisslll",
                                    p->pr_vaddr,
                                    pr_addr_sz,
                                    perms,
                                    name,
                                    (long)p->pr_rss * p->pr_pagesize,
                                    (long)p->pr_anon * p->pr_pagesize,
                                    (long)p->pr_locked * p->pr_pagesize);
        if (!pytuple)
            goto error;
        if (PyList_Append(py_retlist, pytuple))
            goto error;
        Py_DECREF(pytuple);

        
        p += 1;
    }

    close(fd);
    free(xmap);
    return py_retlist;

error:
    if (fd != -1)
        close(fd);
    Py_XDECREF(pytuple);
    Py_DECREF(py_retlist);
    if (xmap != NULL)
        free(xmap);
    return NULL;
}





static PyObject*
get_net_io_counters(PyObject* self, PyObject* args)
{
    kstat_ctl_t    *kc = NULL;
    kstat_t *ksp;
    kstat_named_t *rbytes, *wbytes, *rpkts, *wpkts, *ierrs, *oerrs;

    PyObject* py_retdict = PyDict_New();
    PyObject* py_ifc_info = NULL;

    if (py_retdict == NULL)
        return NULL;
    kc = kstat_open();
    if (kc == NULL)
        goto error;

    ksp = kc->kc_chain;
    while (ksp != NULL) {
        if (ksp->ks_type != KSTAT_TYPE_NAMED)
            goto next;
        if (strcmp(ksp->ks_class, "net") != 0)
            goto next;
        






        if ((strcmp(ksp->ks_module, "link") != 0)) {
            goto next;
        }

        if (kstat_read(kc, ksp, NULL) == -1) {
            errno = 0;
            continue;
        }

        rbytes = (kstat_named_t *)kstat_data_lookup(ksp, "rbytes");
        wbytes = (kstat_named_t *)kstat_data_lookup(ksp, "obytes");
        rpkts = (kstat_named_t *)kstat_data_lookup(ksp, "ipackets");
        wpkts = (kstat_named_t *)kstat_data_lookup(ksp, "opackets");
        ierrs = (kstat_named_t *)kstat_data_lookup(ksp, "ierrors");
        oerrs = (kstat_named_t *)kstat_data_lookup(ksp, "oerrors");

        if ((rbytes == NULL) || (wbytes == NULL) || (rpkts == NULL) ||
            (wpkts == NULL) || (ierrs == NULL) || (oerrs == NULL))
        {
            PyErr_SetString(PyExc_RuntimeError, "kstat_data_lookup() failed");
            goto error;
        }

#if defined(_INT64_TYPE)
        py_ifc_info = Py_BuildValue("(KKKKkkii)", rbytes->value.ui64,
                                                  wbytes->value.ui64,
                                                  rpkts->value.ui64,
                                                  wpkts->value.ui64,
                                                  ierrs->value.ui32,
                                                  oerrs->value.ui32,
#else
        py_ifc_info = Py_BuildValue("(kkkkkkii)", rbytes->value.ui32,
                                                  wbytes->value.ui32,
                                                  rpkts->value.ui32,
                                                  wpkts->value.ui32,
                                                  ierrs->value.ui32,
                                                  oerrs->value.ui32,
#endif
                                                  0,  
                                                  0   
                                    );
        if (!py_ifc_info)
            goto error;
        if (PyDict_SetItemString(py_retdict, ksp->ks_name, py_ifc_info))
            goto error;
        Py_DECREF(py_ifc_info);
        goto next;

        next:
            ksp = ksp->ks_next;
    }

    kstat_close(kc);
    return py_retdict;

error:
    Py_XDECREF(py_ifc_info);
    Py_DECREF(py_retdict);
    if (kc != NULL)
        kstat_close(kc);
    return NULL;
}


#define EXPER_IP_AND_ALL_IRES   (1024+4)


static int PSUTIL_CONN_NONE = 128;









static PyObject*
get_process_connections(PyObject* self, PyObject* args)
{
    long pid;
    int sd = NULL;
    mib2_tcpConnEntry_t *tp = NULL;
    mib2_udpEntry_t     *ude;
#if defined(AF_INET6)
    mib2_tcp6ConnEntry_t *tp6;
    mib2_udp6Entry_t     *ude6;
#endif
    char buf[512];
    int i, flags, getcode, num_ent, state;
    char lip[200], rip[200];
    int lport, rport;
    struct strbuf ctlbuf, databuf;
    struct T_optmgmt_req *tor = (struct T_optmgmt_req *)buf;
    struct T_optmgmt_ack *toa = (struct T_optmgmt_ack *)buf;
    struct T_error_ack   *tea = (struct T_error_ack *)buf;
    struct opthdr        *mibhdr;

    PyObject *py_retlist = PyList_New(0);
    PyObject *py_tuple = NULL;
    PyObject *py_laddr = NULL;
    PyObject *py_raddr = NULL;
    PyObject *af_filter = NULL;
    PyObject *type_filter = NULL;

    if (py_retlist == NULL)
        return NULL;
    if (! PyArg_ParseTuple(args, "lOO", &pid, &af_filter, &type_filter))
        goto error;
    if (!PySequence_Check(af_filter) || !PySequence_Check(type_filter)) {
        PyErr_SetString(PyExc_TypeError, "arg 2 or 3 is not a sequence");
        goto error;
    }

    sd = open("/dev/arp", O_RDWR);
    if (sd == -1) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, "/dev/arp");
        goto error;
    }

    













    
    
    
    
    tor->PRIM_type = T_SVR4_OPTMGMT_REQ;
    tor->OPT_offset = sizeof (struct T_optmgmt_req);
    tor->OPT_length = sizeof (struct opthdr);
    tor->MGMT_flags = T_CURRENT;
    mibhdr = (struct opthdr *)&tor[1];
    mibhdr->level = EXPER_IP_AND_ALL_IRES;
    mibhdr->name  = 0;
    mibhdr->len   = 0;

    ctlbuf.buf = buf;
    ctlbuf.len = tor->OPT_offset + tor->OPT_length;
    flags = 0;  

    if (putmsg(sd, &ctlbuf, (struct strbuf *)0, flags) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    mibhdr = (struct opthdr *)&toa[1];
    ctlbuf.maxlen = sizeof (buf);

    for (;;) {
        flags = 0;
        getcode = getmsg(sd, &ctlbuf, (struct strbuf *)0, &flags);

        if (getcode != MOREDATA ||
            ctlbuf.len < sizeof (struct T_optmgmt_ack) ||
            toa->PRIM_type != T_OPTMGMT_ACK ||
            toa->MGMT_flags != T_SUCCESS)
        {
             break;
        }
        if (ctlbuf.len >= sizeof (struct T_error_ack) &&
            tea->PRIM_type == T_ERROR_ACK)
        {
            PyErr_SetString(PyExc_RuntimeError, "ERROR_ACK");
            goto error;
        }
        if (getcode == 0 &&
            ctlbuf.len >= sizeof (struct T_optmgmt_ack) &&
            toa->PRIM_type == T_OPTMGMT_ACK &&
            toa->MGMT_flags == T_SUCCESS)
        {
            PyErr_SetString(PyExc_RuntimeError, "ERROR_T_OPTMGMT_ACK");
            goto error;
        }

        databuf.maxlen = mibhdr->len;
        databuf.len = 0;
        databuf.buf = (char *)malloc((int)mibhdr->len);
        if (!databuf.buf) {
            
            
            PyErr_NoMemory();
            goto error;
        }

        flags = 0;
        getcode = getmsg(sd, (struct strbuf *)0, &databuf, &flags);
        if (getcode < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto error;
        }

        
        if (mibhdr->level == MIB2_TCP && mibhdr->name == MIB2_TCP_13) {
            tp = (mib2_tcpConnEntry_t *)databuf.buf;
            num_ent = mibhdr->len / sizeof(mib2_tcpConnEntry_t);
            for (i = 0; i < num_ent; i++, tp++) {
                
                if (tp->tcpConnCreationProcess != pid)
                    continue;
                
                inet_ntop(AF_INET, &tp->tcpConnLocalAddress, lip, sizeof(lip));
                inet_ntop(AF_INET, &tp->tcpConnRemAddress, rip, sizeof(rip));
                lport = tp->tcpConnLocalPort;
                rport = tp->tcpConnRemPort;

                
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                if (rport != 0) {
                    py_raddr = Py_BuildValue("(si)", rip, rport);
                }
                else {
                    py_raddr = Py_BuildValue("()");
                }
                if (!py_raddr)
                    goto error;
                state = tp->tcpConnEntryInfo.ce_state;

                
                py_tuple = Py_BuildValue("(iiiNNi)", -1, AF_INET, SOCK_STREAM,
                                                     py_laddr, py_raddr, state);
                if (!py_tuple) {
                    goto error;
                }
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#if defined(AF_INET6)
        
        else if (mibhdr->level == MIB2_TCP6 && mibhdr->name == MIB2_TCP6_CONN) {
            tp6 = (mib2_tcp6ConnEntry_t *)databuf.buf;
            num_ent = mibhdr->len / sizeof(mib2_tcp6ConnEntry_t);

            for (i = 0; i < num_ent; i++, tp6++) {
                
                if (tp6->tcp6ConnCreationProcess != pid)
                    continue;
                
                inet_ntop(AF_INET6, &tp6->tcp6ConnLocalAddress, lip, sizeof(lip));
                inet_ntop(AF_INET6, &tp6->tcp6ConnRemAddress, rip, sizeof(rip));
                lport = tp6->tcp6ConnLocalPort;
                rport = tp6->tcp6ConnRemPort;

                
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                if (rport != 0) {
                    py_raddr = Py_BuildValue("(si)", rip, rport);
                }
                else {
                    py_raddr = Py_BuildValue("()");
                }
                if (!py_raddr)
                    goto error;
                state = tp6->tcp6ConnEntryInfo.ce_state;

                
                py_tuple = Py_BuildValue("(iiiNNi)", -1, AF_INET6, SOCK_STREAM,
                                                     py_laddr, py_raddr, state);

                if (!py_tuple) {
                    goto error;
                }
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#endif
        else if (mibhdr->level == MIB2_UDP || mibhdr->level == MIB2_UDP_ENTRY) {
            ude = (mib2_udpEntry_t *)databuf.buf;
            num_ent = mibhdr->len / sizeof(mib2_udpEntry_t);
            for (i = 0; i < num_ent; i++, ude++) {
                
                if (ude->udpCreationProcess != pid)
                    continue;
                inet_ntop(AF_INET, &ude->udpLocalAddress, lip, sizeof(lip));
                lport = ude->udpLocalPort;
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;
                py_tuple = Py_BuildValue("(iiiNNi)", -1, AF_INET, SOCK_DGRAM,
                                                     py_laddr, py_raddr,
                                                     PSUTIL_CONN_NONE);
                if (!py_tuple) {
                    goto error;
                }
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#if defined(AF_INET6)
        else if (mibhdr->level == MIB2_UDP6 || mibhdr->level == MIB2_UDP6_ENTRY) {
            ude6 = (mib2_udp6Entry_t *)databuf.buf;
            num_ent = mibhdr->len / sizeof(mib2_udp6Entry_t);
            for (i = 0; i < num_ent; i++, ude6++) {
                
                if (ude6->udp6CreationProcess != pid)
                    continue;
                inet_ntop(AF_INET6, &ude6->udp6LocalAddress, lip, sizeof(lip));
                lport = ude6->udp6LocalPort;
                py_laddr = Py_BuildValue("(si)", lip, lport);
                if (!py_laddr)
                    goto error;
                py_raddr = Py_BuildValue("()");
                if (!py_raddr)
                    goto error;
                py_tuple = Py_BuildValue("(iiiNNi)", -1, AF_INET6, SOCK_DGRAM,
                                                     py_laddr, py_raddr,
                                                     PSUTIL_CONN_NONE);
                if (!py_tuple) {
                    goto error;
                }
                if (PyList_Append(py_retlist, py_tuple))
                    goto error;
                Py_DECREF(py_tuple);
            }
        }
#endif
        free(databuf.buf);
    }

    close(sd);
    return py_retlist;

error:
    Py_XDECREF(py_tuple);
    Py_XDECREF(py_laddr);
    Py_XDECREF(py_raddr);
    Py_DECREF(py_retlist);
    
    if (sd != NULL)
        close(sd);
    return NULL;
}





static PyMethodDef
PsutilMethods[] =
{
     
     {"get_process_basic_info", get_process_basic_info, METH_VARARGS,
        "Return process ppid, rss, vms, ctime, nice, nthreads, status and tty"},
     {"get_process_name_and_args", get_process_name_and_args, METH_VARARGS,
        "Return process name and args."},
     {"get_process_cpu_times", get_process_cpu_times, METH_VARARGS,
        "Return process user and system CPU times."},
     {"get_process_cred", get_process_cred, METH_VARARGS,
        "Return process uids/gids."},
     {"query_process_thread", query_process_thread, METH_VARARGS,
        "Return info about a process thread"},
     {"get_process_memory_maps", get_process_memory_maps, METH_VARARGS,
        "Return process memory mappings"},
     {"get_process_num_ctx_switches", get_process_num_ctx_switches, METH_VARARGS,
        "Return the number of context switches performed by process"},
     {"get_process_connections", get_process_connections, METH_VARARGS,
        "Return TCP and UDP connections opened by process."},

     
     {"get_swap_mem", get_swap_mem, METH_VARARGS,
        "Return information about system swap memory."},
     {"get_system_users", get_system_users, METH_VARARGS,
        "Return currently connected users."},
     {"get_disk_partitions", get_disk_partitions, METH_VARARGS,
        "Return disk partitions."},
     {"get_system_per_cpu_times", get_system_per_cpu_times, METH_VARARGS,
        "Return system per-CPU times."},
     {"get_disk_io_counters", get_disk_io_counters, METH_VARARGS,
        "Return a Python dict of tuples for disk I/O statistics."},
     {"get_net_io_counters", get_net_io_counters, METH_VARARGS,
        "Return a Python dict of tuples for network I/O statistics."},

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
psutil_sunos_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int
psutil_sunos_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

static struct PyModuleDef
moduledef = {
    PyModuleDef_HEAD_INIT,
    "psutil_sunos",
    NULL,
    sizeof(struct module_state),
    PsutilMethods,
    NULL,
    psutil_sunos_traverse,
    psutil_sunos_clear,
    NULL
};

#define INITERROR return NULL

PyObject *
PyInit__psutil_sunos(void)

#else
#define INITERROR return

void init_psutil_sunos(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("_psutil_sunos", PsutilMethods);
#endif
    PyModule_AddIntConstant(module, "SSLEEP", SSLEEP);
    PyModule_AddIntConstant(module, "SRUN", SRUN);
    PyModule_AddIntConstant(module, "SZOMB", SZOMB);
    PyModule_AddIntConstant(module, "SSTOP", SSTOP);
    PyModule_AddIntConstant(module, "SIDL", SIDL);
    PyModule_AddIntConstant(module, "SONPROC", SONPROC);
    PyModule_AddIntConstant(module, "SWAIT", SWAIT);

    PyModule_AddIntConstant(module, "PRNODEV", PRNODEV);  

    PyModule_AddIntConstant(module, "TCPS_CLOSED", TCPS_CLOSED);
    PyModule_AddIntConstant(module, "TCPS_CLOSING", TCPS_CLOSING);
    PyModule_AddIntConstant(module, "TCPS_CLOSE_WAIT", TCPS_CLOSE_WAIT);
    PyModule_AddIntConstant(module, "TCPS_LISTEN", TCPS_LISTEN);
    PyModule_AddIntConstant(module, "TCPS_ESTABLISHED", TCPS_ESTABLISHED);
    PyModule_AddIntConstant(module, "TCPS_SYN_SENT", TCPS_SYN_SENT);
    PyModule_AddIntConstant(module, "TCPS_SYN_RCVD", TCPS_SYN_RCVD);
    PyModule_AddIntConstant(module, "TCPS_FIN_WAIT_1", TCPS_FIN_WAIT_1);
    PyModule_AddIntConstant(module, "TCPS_FIN_WAIT_2", TCPS_FIN_WAIT_2);
    PyModule_AddIntConstant(module, "TCPS_LAST_ACK", TCPS_LAST_ACK);
    PyModule_AddIntConstant(module, "TCPS_TIME_WAIT", TCPS_TIME_WAIT);
    PyModule_AddIntConstant(module, "TCPS_IDLE", TCPS_IDLE);  
    PyModule_AddIntConstant(module, "TCPS_BOUND", TCPS_BOUND);  
    PyModule_AddIntConstant(module, "PSUTIL_CONN_NONE", PSUTIL_CONN_NONE);

    if (module == NULL) {
        INITERROR;
    }
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
