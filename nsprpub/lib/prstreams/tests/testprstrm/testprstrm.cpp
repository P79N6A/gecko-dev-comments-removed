




































#include "prinit.h"
#include "prstrms.h"
#include "prio.h"
#include <string.h>
#include <stdio.h>
#if defined(XP_UNIX) || defined(XP_OS2)
#include <sys/types.h>
#include <sys/stat.h>
#endif

const unsigned int MaxCnt = 1;

void threadwork(void *mytag);


typedef struct threadarg {
    void *mytag;
} threadarg;

void 
threadmain(void *mytag)
{
    threadarg arg;

    arg.mytag = mytag;

    threadwork(&arg);
}


void
threadwork(void *_arg)
{
	threadarg *arg = (threadarg *)_arg;
	unsigned int i;

	char fname1[256];
	char fname2[256];

	strcpy(fname1, (char *)arg->mytag);
	strcpy(fname2, (char *)arg->mytag);
	strcat(fname2, "2");
	PR_Delete(fname1);
	PR_Delete(fname2);

	PRfilebuf *fb[MaxCnt];
	PRifstream *ifs[MaxCnt];
	PRofstream *ofs[MaxCnt];
	int mode = 0;
#ifdef XP_UNIX
	mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IRGRP|S_IWOTH|S_IROTH;
#endif

	
	
	cout << "Testing unused filebufs ----------------" << endl;
	for (i=0; i < MaxCnt; i++){
		fb[i] = new PRfilebuf;
	}
	
	for (i=0; i < MaxCnt; i++){
		delete fb[i];
	}
	cout << "Unused filebufs complete ---------------" << endl;

	
	
	cout << "Testing unused ifstream -----------------" << endl;
	for (i=0; i < MaxCnt; i++){
	  ifs[i] = new PRifstream;
	}
	
	
	for (i=0; i < MaxCnt; i++){
	  delete ifs[i];
	}
	cout << "Unused ifstream complete ----------------" << endl;
	
	
	cout << "Testing unused ofstream -----------------" << endl;
	for (i=0; i < MaxCnt; i++){
		ofs[i] = new PRofstream;
	}
	for (i=0; i < MaxCnt; i++){
	  *(ofs[i]) << "A"; 
	  delete ofs[i]; 
	}
	cout << "Unused ofstream complete ----------------" << endl;

	cout << "Testing use of ofstream 1 (extra filebuf allocated) ---------" << endl;
	PRofstream *aos = new PRofstream(fname1, ios::out|ios::ate, mode);
	for (i=0; i < MaxCnt; i++){
	  for (int j=0; j < 8192; j++)
		*aos << "AaBbCcDdEeFfGg" << endl;
		fb[i] = new PRfilebuf; 
	}
	
	
	for (i=0; i < MaxCnt; i++){
	  delete fb[i];
	}
	aos->flush(); 
	delete aos;
	cout << "Testing use of ofstream 1 complete (extra filebuf deleted) --" << endl;
	cout << "Testing use of ofstream 2 (extra filebuf allocated) ---------" << endl;
	PRofstream *aos2 = new PRofstream(fname2, ios::out, mode);

	for (i=0; i < MaxCnt; i++){
	    *aos2 << "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
	}
	
	delete aos2;
	cout << "Testing use of ofstream 2 complete (extra filebuf deleted) --" << endl;
	char line[1024];
	cout << "Testing use of ifstream 1 (stack allocation) -------------" << endl;
	PRifstream ais(fname1);
	for (i=0; i < MaxCnt; i++){
		ais >> line;
	}
	cout << "Testing use of ifstream 1 complete -----------------------" << endl;
	cout << "Testing use of ifstream 2 ----------------------" << endl;
	PRifstream *ais2 = new PRifstream(fname2);
	char achar;
	for (i=0; i < MaxCnt*10; i++){
		*ais2 >> achar;
	}
	delete ais2;
	cout << "Testing use of ifstream 2 complete -------------" << endl;
}

#define STACKSIZE 1024*1024
int
main(int argc, char **argv)
{
	PR_Init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 256);
	threadmain("TestFile");
	PRThread *thr1 = PR_CreateThread(PR_SYSTEM_THREAD, 
					 threadmain, 
					 (void *)"TestFile1",
					 PR_PRIORITY_NORMAL,
					 PR_GLOBAL_THREAD,
					 PR_JOINABLE_THREAD,
					 STACKSIZE);
	PRThread *thr2 = PR_CreateThread(PR_SYSTEM_THREAD, 
					 threadmain, 
					 (void *)"TestFile2",
					 PR_PRIORITY_NORMAL,
					 PR_GLOBAL_THREAD,
					 PR_JOINABLE_THREAD,
					 STACKSIZE);

	PRThread *thr3 = PR_CreateThread(PR_SYSTEM_THREAD, 
					 threadmain, 
					 (void *)"TestFile3",
					 PR_PRIORITY_NORMAL,
					 PR_GLOBAL_THREAD,
					 PR_JOINABLE_THREAD,
					 STACKSIZE);
	PR_JoinThread(thr1);
	PR_JoinThread(thr2);
	PR_JoinThread(thr3);
	return 0;
}

