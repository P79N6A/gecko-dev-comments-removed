



#include "sll_lite.h"


#define CCAPP_CCPROVIER     1
#define CCAPP_MSPROVIDER    2

typedef void(* appListener) (void *message, int type);
typedef struct {
    sll_lite_node_t node;
    int type;
    appListener *listener_p;
} listener_t;

extern void addCcappListener(appListener* listener, int type);
appListener *getCcappListener(int type);
cpr_status_e ccappTaskSendMsg (uint32_t cmd, void *msg, uint16_t len, uint32_t usrInfo);
