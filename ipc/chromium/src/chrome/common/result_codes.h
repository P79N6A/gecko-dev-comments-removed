



#ifndef CHROME_COMMON_RESULT_CODES_H_
#define CHROME_COMMON_RESULT_CODES_H_

#include "base/process_util.h"











class ResultCodes {
 public:
  enum ExitCode {
    NORMAL_EXIT = base::PROCESS_END_NORMAL_TERMINATON,
    TASKMAN_KILL = base::PROCESS_END_KILLED_BY_USER,
    HUNG = base::PROCESS_END_PROCESS_WAS_HUNG,
    INVALID_CMDLINE_URL,        
    SBOX_INIT_FAILED,           
    GOOGLE_UPDATE_INIT_FAILED,  
    GOOGLE_UPDATE_LAUNCH_FAILED,
    BAD_PROCESS_TYPE,           
    MISSING_PATH,               
    MISSING_DATA,               
    SHELL_INTEGRATION_FAILED,   
    MACHINE_LEVEL_INSTALL_EXISTS, 
    UNINSTALL_DELETE_FILE_ERROR,
    UNINSTALL_CHROME_ALIVE,     
    UNINSTALL_NO_SURVEY,        
    UNINSTALL_USER_CANCEL,      
    UNINSTALL_DELETE_PROFILE,   
    UNSUPPORTED_PARAM,          
    KILLED_BAD_MESSAGE,         
    IMPORTER_CANCEL,            
    IMPORTER_HUNG,              
    EXIT_LAST_CODE              
  };
};

#endif
