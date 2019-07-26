






































#ifndef _util_h
#define _util_h

#include "registry.h"

int nr_get_filename(char *base,char *name, char **namep);
#if 0
#include <openssl/ssl.h>

int read_RSA_private_key(char *base, char *name,RSA **keyp);
#endif
void nr_errprintf_log(const char *fmt,...);
void nr_errprintf_log2(void *ignore, const char *fmt,...);
extern int nr_util_default_log_facility;

int nr_read_data(int fd,char *buf,int len);
int nr_drop_privileges(char *username);
int nr_hex_ascii_dump(Data *data);
int nr_fwrite_all(FILE *fp,UCHAR *buf,int len);
int nr_sha1_file(char *filename,UCHAR *out);
int nr_bin2hex(UCHAR *in,int len,UCHAR *out);
int nr_rm_tree(char *path);
int nr_write_pid_file(char *pid_filename);

int nr_reg_uint4_fetch_and_check(NR_registry key, UINT4 min, UINT4 max, int log_fac, int die, UINT4 *val);
int nr_reg_uint8_fetch_and_check(NR_registry key, UINT8 min, UINT8 max, int log_fac, int die, UINT8 *val);

#ifdef WIN32
int snprintf(char *buffer, size_t n, const char *format, ...);
const char *inet_ntop(int af, const void *src, char *dst, size_t size);
#endif

#endif

