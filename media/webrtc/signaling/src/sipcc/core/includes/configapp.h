



#ifndef CONFIGAPP_H
#define CONFIGAPP_H

extern void configapp_init();
extern void configapp_shutdown();
extern void configapp_process_msg(uint32_t cmd, void *msg);

#endif

