








#ifndef _UTILS_LOGGER_H
#define _UTILS_LOGGER_H

#include <stdint.h>






struct logger_entry {
    uint16_t    len;    
    uint16_t    __pad;  
    int32_t     pid;    
    int32_t     tid;    
    int32_t     sec;    
    int32_t     nsec;   
    char        msg[0]; 
};






struct logger_entry_v2 {
    uint16_t    len;       
    uint16_t    hdr_size;  
    int32_t     pid;       
    int32_t     tid;       
    int32_t     sec;       
    int32_t     nsec;      
    uint32_t    euid;      
    char        msg[0];    
};

#define LOGGER_LOG_MAIN		"log/main"
#define LOGGER_LOG_RADIO	"log/radio"
#define LOGGER_LOG_EVENTS	"log/events"
#define LOGGER_LOG_SYSTEM	"log/system"







#define LOGGER_ENTRY_MAX_PAYLOAD	4076






#define LOGGER_ENTRY_MAX_LEN		(5*1024)

#ifdef HAVE_IOCTL

#include <sys/ioctl.h>

#define __LOGGERIO	0xAE

#define LOGGER_GET_LOG_BUF_SIZE		_IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN		_IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN	_IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG		_IO(__LOGGERIO, 4) /* flush log */
#define LOGGER_GET_VERSION		_IO(__LOGGERIO, 5) /* abi version */
#define LOGGER_SET_VERSION		_IO(__LOGGERIO, 6) /* abi version */

#endif 

#endif 
