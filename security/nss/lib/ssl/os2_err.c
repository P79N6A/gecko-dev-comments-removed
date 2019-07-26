













#include "prerror.h"
#include "prlog.h"
#include <errno.h>








void nss_MD_os2_map_default_error(PRInt32 err);

void nss_MD_os2_map_opendir_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_closedir_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_readdir_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_delete_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}


void nss_MD_os2_map_stat_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_fstat_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_rename_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}


void nss_MD_os2_map_access_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_mkdir_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_rmdir_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_read_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_transmitfile_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_write_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_lseek_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_fsync_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}




void nss_MD_os2_map_close_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_socket_error(PRInt32 err)
{

    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_recv_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_recvfrom_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_send_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {

    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_sendto_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {

    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_accept_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {


    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_acceptex_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_connect_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {



    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_bind_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {

    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_listen_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {


    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_shutdown_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_getsockname_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {

    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_getpeername_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_getsockopt_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_setsockopt_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_open_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}

void nss_MD_os2_map_gethostname_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}




void nss_MD_os2_map_select_error(PRInt32 err)
{
    PRErrorCode prError;
    switch (err) {

    default:		nss_MD_os2_map_default_error(err); return;
    }
    PR_SetError(prError, err);
}

void nss_MD_os2_map_lockf_error(PRInt32 err)
{
    nss_MD_os2_map_default_error(err);
}



void nss_MD_os2_map_default_error(PRInt32 err)
{
    PRErrorCode prError;

    switch (err) {











#if ERROR_FILE_NOT_FOUND != ENOENT

#endif
    default: 			prError = PR_UNKNOWN_ERROR; break;
    }
    PR_SetError(prError, err);
}

