#if 0
#define NR_LOG_REGISTRY BLAHBLAH()
#define LOG_REGISTRY BLAHBLAH()
static int BLAHBLAH() {
int blahblah;
r_log_register("registry",&blahblah);
return blahblah;
}
#endif













































#ifndef __REGISTRY_INT_H__
#define __REGISTRY_INT_H__

#include <sys/types.h>
#include <r_types.h>
#ifndef NO_REG_RPC
#include <rpc/rpc.h>
#endif

extern int NR_LOG_REGISTRY;

int nr_reg_is_valid(NR_registry name);

#define NR_REG_TYPE_CHAR               0
#define NR_REG_TYPE_UCHAR              1
#define NR_REG_TYPE_INT2               2
#define NR_REG_TYPE_UINT2              3
#define NR_REG_TYPE_INT4               4
#define NR_REG_TYPE_UINT4              5
#define NR_REG_TYPE_INT8               6
#define NR_REG_TYPE_UINT8              7
#define NR_REG_TYPE_DOUBLE             8
#define NR_REG_TYPE_BYTES              9
#define NR_REG_TYPE_STRING             10
#define NR_REG_TYPE_REGISTRY           11
char *nr_reg_type_name(int type);
int nr_reg_compute_type(char *type_name, int *type);

char *nr_reg_action_name(int action);

int nr_reg_cb_init(void);
int nr_reg_client_cb_init(void);
int nr_reg_register_for_callbacks(int fd, int connect_to_port);
int nr_reg_raise_event(NR_registry name, int action);
#ifndef NO_REG_RPC
int nr_reg_get_client(CLIENT **client);
#endif

#define CALLBACK_SERVER_ADDR     "127.0.0.1"
#define CALLBACK_SERVER_PORT     8082
#define CALLBACK_SERVER_BACKLOG  32

#endif
