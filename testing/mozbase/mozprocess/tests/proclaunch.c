



#include <stdio.h>
#include <stdlib.h>
#include "iniparser.h"

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>

extern int iniparser_getint(dictionary *d, char *key, int notfound);
extern char *iniparser_getstring(dictionary *d, char *key, char *def);


int launchWindows(int children, int maxtime) {
  _TCHAR cmdline[50];
  STARTUPINFO startup;
  PROCESS_INFORMATION procinfo;
  BOOL rv = 0;
  
  _stprintf(cmdline, _T("proclaunch.exe %d %d"), children, maxtime);
  ZeroMemory(&startup, sizeof(STARTUPINFO));
  startup.cb = sizeof(STARTUPINFO);
  
  ZeroMemory(&procinfo, sizeof(PROCESS_INFORMATION));
  
  printf("Launching process!\n");
  rv = CreateProcess(NULL,
                cmdline,
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &startup,
                &procinfo);

  if (!rv) {
    DWORD dw = GetLastError(); 
    printf("error: %d\n", dw); 
  }
  CloseHandle(procinfo.hProcess);
  CloseHandle(procinfo.hThread);
  return 0;
}
#endif

int main(int argc, char **argv) {
  int children = 0;
  int maxtime = 0;
  int passedtime = 0;
  dictionary *dict = NULL;

  
  if (argc == 1 || (0 == strcmp(argv[1], "-h")) || (0 == strcmp(argv[1], "--help"))) {
    printf("ProcLauncher takes an ini file.  Specify the ini file as the only\n");
    printf("parameter of the command line:\n");
    printf("proclauncher my.ini\n\n");
    printf("The ini file has the form:\n");
    printf("[main]\n");
    printf("children=child1,child2  ; These comma separated values are sections\n");
    printf("maxtime=60              ; Max time this process lives\n");
    printf("[child1]                ; Here is a child section\n");
    printf("children=3              ; You can have grandchildren: this spawns 3 of them for child1\n");
    printf("maxtime=30              ; Max time, note it's in seconds. If this time\n");
    printf("                        ; is > main:maxtime then the child process will be\n");
    printf("                        ; killed when the parent exits. Also, grandchildren\n");
    printf("[child2]                ; inherit this maxtime and can't change it.\n");
    printf("maxtime=25              ; You can call these sections whatever you want\n");
    printf("children=0              ; as long as you reference them in a children attribute\n");
    printf("....\n");
    return 0;
  } else if (argc == 2) {
    
    
    dict = iniparser_load(argv[1]);
    
  } else if (argc == 3) {
    
    
    children = atoi(argv[1]);
    maxtime = atoi(argv[2]);
  }

  if (dict) {
    
    char *childlist = iniparser_getstring(dict, "main:children", NULL);
    maxtime = iniparser_getint(dict, (char*)"main:maxtime", 10);;
	if (childlist) {
      int c = 0, m = 10;
      char childkey[50], maxkey[50];
      char cmd[25];
      char *token = strtok(childlist, ",");

      while (token) {
        
        memset(childkey, 0, 50);
        memset(maxkey, 0, 50);
        memset(cmd, 0, 25);
        c = 0;
        m = 10;

        sprintf(childkey, "%s:children", token);
        sprintf(maxkey, "%s:maxtime", token);
        c = iniparser_getint(dict, childkey, 0);
        m = iniparser_getint(dict, maxkey, 10);
        
        
        #ifdef _WIN32
          launchWindows(c, m);
        #else
          sprintf(cmd, "./proclaunch %d %d &", c, m);
          system(cmd);
        #endif

        
        token = strtok(NULL, ",");
      }
    }
    iniparser_freedict(dict);
  } else {
    
    char cmd[25];
    
    
    #ifdef _WIN32
      while(children > 0) {
        launchWindows(0, maxtime);
        children--;
      }
    #else
      sprintf(cmd, "./proclaunch %d %d &", 0, maxtime); 
      printf("Launching child process: %s\n", cmd);
      while (children  > 0) {
        system(cmd);
        children--;
      }
    #endif
  }

  

  while (passedtime < maxtime) {
#ifdef _WIN32
		Sleep(1000);
#else
	    sleep(1);
#endif
    passedtime++;
  }
  exit(0);
  return 0;
}
