






































#ifndef _XI_ERRORS_H_
#define _XI_ERRORS_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <gtk/gtk.h>




    enum
    {
        OK              = 0,
        E_MEM           = -601,     
        E_PARAM         = -602,     
        E_NO_MEMBER     = -603,     
        E_INVALID_PTR   = -604,     
        E_INVALID_KEY   = -605,     
        E_EMPTY_README  = -606,     
        E_EMPTY_LICENSE = -607,     
        E_OUT_OF_BOUNDS = -608,     
        E_REF_COUNT     = -609,     
        E_NO_COMPONENTS = -610,     
        E_NO_SETUPTYPES = -611,     
        E_URL_ALREADY   = -612,     
        E_DIR_CREATE    = -613,     
        E_BAD_FTP_URL   = -614,     
        E_NO_DOWNLOAD   = -615,     
        E_EXTRACTION    = -616,     
        E_FORK_FAILED   = -617,     
        E_LIB_OPEN      = -618,     
        E_LIB_SYM       = -619,     
        E_XPI_FAIL      = -620,     
        E_INSTALL       = -621,     
        E_CP_FAIL       = -622,     
        E_NO_DEST       = -623,     
        E_MKDIR_FAIL    = -624,     
        E_OLD_INST      = -625,     
        E_NO_PERMS      = -626,     
        E_NO_DISK_SPACE = -627,     
        E_CRC_FAILED    = -628,     
        E_USAGE_SHOWN   = -629,     
        E_DIR_NOT_EMPTY = -630,     
        E_INVALID_PROXY = -631      
    };

#endif 
