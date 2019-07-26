






































#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "config.h"
#include "dns_utils.h"
#include "phone_debug.h"
#include "ccapi.h"
#include "debug.h"

cc_int32_t ConfigDebug;
























void
config_get_string (int id, char *buffer, int buffer_len)
{
    const var_t *entry;
    char *buf_start;

    


    buffer[0] = 0;
    if ((id >= 0) && (id < CFGID_PROTOCOL_MAX)) {
        entry = &prot_cfg_table[id];
        if (entry->length > buffer_len) {
            CONFIG_ERROR(CFG_F_PREFIX"insufficient buffer: %d\n", "config_get_string", 
                    id);
        } else {
            buf_start = buffer;
            entry->print_func(entry, buffer, buffer_len);
            CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: get str: %s = %s\n", DEB_F_PREFIX_ARGS(CONFIG_API, "config_get_string"), id, entry->name,
                         buf_start);
        }
    } else {
        CONFIG_ERROR(CFG_F_PREFIX"Invalid ID: %d\n", "config_get_string", id);
    }
}












void
config_set_string (int id, char *buffer)
{
    const var_t *entry;

    if ((id >= 0) && (id < CFGID_PROTOCOL_MAX)) {
        entry = &prot_cfg_table[id];
        if (entry->parse_func(entry, buffer)) {
            
            CONFIG_ERROR(CFG_F_PREFIX"Parse function failed. ID: %d %s:%s\n", "config_set_string", id, entry->name, buffer);
        } else {
            CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s set str to %s\n", DEB_F_PREFIX_ARGS(CONFIG_API, "config_set_string"), id, entry->name,
                         buffer);
        }
    } else {
        CONFIG_ERROR(CFG_F_PREFIX"Invalid ID: %d\n", "config_set_string", id);
    }
}

#define MAX_CONFIG_VAL_PRINT_LEN 256





















void
print_config_value (int id, char *get_set, const char *entry_name,
                    void *buffer, int length)
{
    long  long_val  = 0;
    int   int_val   = 0;
    short short_val = 0;
    char  char_val  = 0;
    char  str[MAX_CONFIG_VAL_PRINT_LEN];
    char *in_ptr;
    char *str_ptr;

    if (length == sizeof(char)) {
        char_val = *(char *) buffer;
        long_val = (long) char_val;
        CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s: %s = %ld\n", DEB_F_PREFIX_ARGS(CONFIG_API, "print_config_value"), id, get_set, entry_name,
                     long_val);
    } else if (length == sizeof(short)) {
        short_val = *(short *) buffer;
        long_val = (long) short_val;
        CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s: %s = %ld\n", DEB_F_PREFIX_ARGS(CONFIG_API, "print_config_value"), id, get_set, entry_name,
                     long_val);
    } else if (length == sizeof(int)) {
        int_val = *(int *) buffer;
        long_val = (long) int_val;
        CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s: %s = %ld\n", DEB_F_PREFIX_ARGS(CONFIG_API, "print_config_value"), id, get_set, entry_name,
                     long_val);
    } else if (length == sizeof(long)) {
        long_val = *(long *) buffer;
        CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s: %s = %ld\n", DEB_F_PREFIX_ARGS(CONFIG_API, "print_config_value"), id, get_set, entry_name,
                     long_val);
    } else if (length < MAX_CONFIG_VAL_PRINT_LEN / 2) {

        in_ptr = (char *) buffer;
        str_ptr = &str[0];
        while (length--) {
            sprintf(str_ptr++, "%02x", *in_ptr++);
            str_ptr++;
        }
        *str_ptr = '\0';
        CONFIG_DEBUG(DEB_F_PREFIX"CFGID %d: %s: %s = %s\n", DEB_F_PREFIX_ARGS(CONFIG_API, "print_config_value"), id, get_set, entry_name, str);
    } else {
        CONFIG_ERROR(CFG_F_PREFIX"cfg_id = %d length too long -> %d\n", "print_config_value", 
                id, length);
    }
}














void
config_get_value (int id, void *buffer, int length)
{
    const var_t *entry;

    


    if ((id >= 0) && (id < CFGID_PROTOCOL_MAX)) {
        entry = &prot_cfg_table[id];
        if (length == entry->length) {
            memcpy(buffer, entry->addr, entry->length);

            if (ConfigDebug) {
                print_config_value(id, "Get Val", entry->name, buffer, length);
            }
        } else {
            CONFIG_ERROR(CFG_F_PREFIX"%s size error\n", "config_get_value", 
                    entry->name);
        }
    } else {
        CONFIG_ERROR(CFG_F_PREFIX"Invalid ID: %d\n", "config_get_value", id);
    }
}















void
config_set_value (int id, void *buffer, int length)
{
    const var_t *entry;

    


    if ((id >= 0) && (id < CFGID_PROTOCOL_MAX)) {
        entry = &prot_cfg_table[id];
        if (entry->length != length) {
            CONFIG_ERROR(CFG_F_PREFIX" %s size error entry size=%d, len=%d\n",
                    "config_set_value", entry->name, entry->length, length);
            return;
        }
        memcpy(entry->addr, buffer, entry->length);
        if (ConfigDebug) {
            print_config_value(id, "Set Val", entry->name, buffer, length);
        }
    } else {
        CONFIG_ERROR(CFG_F_PREFIX"Invalid ID: %d\n", "config_set_value", id);
    }
}









char *
get_printable_cfg(unsigned int indx, char *buf, unsigned int len)
{
   const var_t *table;
   buf[0]=0;
   
   table = &prot_cfg_table[indx];
   
   
   if (indx>=CFGID_LINE_PASSWORD && indx < CFGID_LINE_PASSWORD+MAX_CONFIG_LINES) {
     
     strncpy(buf, "**********", MAX_CONFIG_VAL_PRINT_LEN);
   } else if ( table->print_func ) {
     table->print_func(table, buf, len);
   }

   if ( buf[0] == 0 ) {
     strcpy(buf,"EMPTY");
   }
   return buf;
}











cc_int32_t
show_config_cmd (cc_int32_t argc, const char *argv[])
{
    const var_t *table;
    char buf[MAX_CONFIG_VAL_PRINT_LEN];
    int i, feat;

    debugif_printf("\n------ Current *Cache* Configuration ------\n");
    table = prot_cfg_table;

    for ( i=0; i < CFGID_LINE_FEATURE; i++ ) {
        if (table->print_func) {
            table->print_func(table, buf, sizeof(buf));

            
            
            if (strstr(table->name, "Password") != 0) {
                
                strncpy(buf, "**********", MAX_CONFIG_VAL_PRINT_LEN);
            }
            debugif_printf("%s : %s\n", table->name, buf);
        }
        table++;
    }

    debugif_printf("%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
        prot_cfg_table[CFGID_LINE_INDEX].name,
        prot_cfg_table[CFGID_LINE_FEATURE].name,
        prot_cfg_table[CFGID_LINE_MAXNUMCALLS].name,
        prot_cfg_table[CFGID_LINE_BUSY_TRIGGER].name,
        prot_cfg_table[CFGID_PROXY_ADDRESS].name,
        prot_cfg_table[CFGID_PROXY_PORT].name,
        prot_cfg_table[CFGID_LINE_CALL_WAITING].name,
        prot_cfg_table[CFGID_LINE_MSG_WAITING_LAMP].name,
        prot_cfg_table[CFGID_LINE_MESSAGE_WAITING_AMWI].name,
        prot_cfg_table[CFGID_LINE_RING_SETTING_IDLE].name,
        prot_cfg_table[CFGID_LINE_RING_SETTING_ACTIVE].name,
        prot_cfg_table[CFGID_LINE_NAME].name,
        prot_cfg_table[CFGID_LINE_AUTOANSWER_ENABLED].name,
        prot_cfg_table[CFGID_LINE_AUTOANSWER_MODE].name,
        prot_cfg_table[CFGID_LINE_AUTHNAME].name,
        prot_cfg_table[CFGID_LINE_PASSWORD].name,
        prot_cfg_table[CFGID_LINE_DISPLAYNAME].name,
        prot_cfg_table[CFGID_LINE_CONTACT].name);

    for (i=0; i< MAX_CONFIG_LINES; i++) {
      config_get_value(CFGID_LINE_FEATURE+i, &feat, sizeof(feat));
      if ( feat != CC_FEATURE_NONE ){
        debugif_printf("%3s ", get_printable_cfg(CFGID_LINE_INDEX+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%4s ", get_printable_cfg(CFGID_LINE_FEATURE+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%3s ", get_printable_cfg(CFGID_LINE_MAXNUMCALLS+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%3s ", get_printable_cfg(CFGID_LINE_BUSY_TRIGGER+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%12s ", get_printable_cfg(CFGID_PROXY_ADDRESS+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_PROXY_PORT+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%3s ", get_printable_cfg(CFGID_LINE_CALL_WAITING+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%6s ", get_printable_cfg(CFGID_LINE_MSG_WAITING_LAMP+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%6s ", get_printable_cfg(CFGID_LINE_MESSAGE_WAITING_AMWI+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%6s ", get_printable_cfg(CFGID_LINE_RING_SETTING_IDLE+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%6s ", get_printable_cfg(CFGID_LINE_RING_SETTING_ACTIVE+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("     %s ", get_printable_cfg(CFGID_LINE_NAME+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_LINE_AUTOANSWER_ENABLED+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_LINE_AUTOANSWER_MODE+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_LINE_AUTHNAME+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_LINE_PASSWORD+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s ", get_printable_cfg(CFGID_LINE_DISPLAYNAME+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
        debugif_printf("%s\n", get_printable_cfg(CFGID_LINE_CONTACT+i, buf, MAX_CONFIG_VAL_PRINT_LEN));
      }
    }
    
    return (0);
}



















static int
config_get_line_id (int id, int line)
{
    int line_id = 0;
    const var_t *entry;

    if ((line == 0) || (line > MAX_REG_LINES)) {
        entry = &prot_cfg_table[id];  
        (void) entry;
        CONFIG_ERROR(CFG_F_PREFIX"ID=%d- line %d out of range\n", "config_get_line_id", id, line);
        return (0);
    }
    line_id = id + line - 1;

    return (line_id);
}














void
config_get_line_string (int id, char *buffer, int line, int buffer_len)
{
    int line_id = 0;

    line_id = config_get_line_id(id, line);
    if (line_id) {
        config_get_string(line_id, buffer, buffer_len);
    }
}













void
config_set_line_string (int id, char *buffer, int line)
{
    int line_id = 0;

    line_id = config_get_line_id(id, line);
    if (line_id) {
        config_set_string(line_id, buffer);
    }
}















void
config_get_line_value (int id, void *buffer, int length, int line)
{
    int line_id = 0;

    line_id = config_get_line_id(id, line);
    if (line_id) {
        config_get_value(line_id, buffer, length);
    }
}















void
config_set_line_value (int id, void *buffer, int length, int line)
{
    int line_id = 0;

    line_id = config_get_line_id(id, line);
    if (line_id) {
        config_set_value(line_id, buffer, length);
    }
}











void
config_init (void)
{
    
}
