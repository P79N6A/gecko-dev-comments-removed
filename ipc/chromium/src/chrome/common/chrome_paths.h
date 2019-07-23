



#ifndef CHROME_COMMON_CHROME_PATHS_H__
#define CHROME_COMMON_CHROME_PATHS_H__




namespace chrome {

enum {
  PATH_START = 1000,

  DIR_APP = PATH_START,  
  DIR_LOGS,              
  DIR_USER_DATA,         
  DIR_CRASH_DUMPS,       
  DIR_USER_DESKTOP,      
  DIR_RESOURCES,         
  DIR_INSPECTOR,         
  DIR_THEMES,            
  DIR_LOCALES,           
  DIR_APP_DICTIONARIES,  
  DIR_USER_DOCUMENTS,    
  DIR_DEFAULT_DOWNLOADS, 
  FILE_RESOURCE_MODULE,  
                         
  FILE_LOCAL_STATE,      
                         
  FILE_RECORDED_SCRIPT,  
                         
  FILE_GEARS_PLUGIN,     
  FILE_LIBAVCODEC,       
  FILE_LIBAVFORMAT,      
  FILE_LIBAVUTIL,        

  
  DIR_TEST_DATA,         
  DIR_TEST_TOOLS,        
  FILE_TEST_SERVER,      
  FILE_PYTHON_RUNTIME,   

  PATH_END
};


void RegisterPathProvider();

}  

#endif  
