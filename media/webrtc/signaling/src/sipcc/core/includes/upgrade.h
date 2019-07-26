






































#ifndef _UPGRADE_INCLUDED_H
#define _UPGRADE_INCLUDED_H

#include "cpr_types.h"
#include "phone.h"








typedef struct
{
    uint8_t vectors[0x20];
    LoadHdr hdr;
} BigLoadHdr;




void upgrade_bootup_init(void);
int upgrade_done(int tftp_rc);
void upgrade_start(const char *fname);
void upgrade_memcpy(void *dst, const void *src, int len);
int upgrade_check(const char *loadid);
int upgrade_validate_app_image(const BigLoadHdr *load);
int upgrade_validate_dsp_image(const DSPLoadHdr *dsp);
void upgrade_erase_dir_storage(void);
void upgrade_write_dir_storage(char *buffer, char *flash, int size);

#endif 
