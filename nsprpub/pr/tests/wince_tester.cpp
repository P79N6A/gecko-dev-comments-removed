



































#ifdef WINCE


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef int (*pMainFnc) (int argc, char **argv);


#define MAX_ARG_LENGTH		2048




WCHAR * skip_until = TEXT(""); 



typedef struct TEST_MODULE_ITEM_NAME {
    WCHAR *pW;
    CHAR  *pC;
} TestModuleItemName, *pTestModuleItemName;

#define TEST_MODULE_ITEM(x)  { TEXT(x), x }
#define END_POINT_MARKER     { NULL, NULL }


TestModuleItemName test_module_list[] =
{
    TEST_MODULE_ITEM("accept"),
    TEST_MODULE_ITEM("acceptread"),
    TEST_MODULE_ITEM("acceptreademu"),
    TEST_MODULE_ITEM("affinity"),
    TEST_MODULE_ITEM("alarm"),
    TEST_MODULE_ITEM("anonfm"),
    TEST_MODULE_ITEM("atomic"),
    TEST_MODULE_ITEM("attach"),
    TEST_MODULE_ITEM("bigfile"),
    TEST_MODULE_ITEM("cleanup"),
    TEST_MODULE_ITEM("cltsrv"),
    TEST_MODULE_ITEM("concur"),
    TEST_MODULE_ITEM("cvar"),
    TEST_MODULE_ITEM("cvar2"),
    TEST_MODULE_ITEM("dlltest"),
    TEST_MODULE_ITEM("dtoa"),
    TEST_MODULE_ITEM("errcodes"),
    TEST_MODULE_ITEM("exit"),
    TEST_MODULE_ITEM("fdcach"),
    TEST_MODULE_ITEM("fileio"),
    TEST_MODULE_ITEM("foreign"),
    TEST_MODULE_ITEM("formattm"),
    TEST_MODULE_ITEM("fsync"),
    TEST_MODULE_ITEM("gethost"),
    TEST_MODULE_ITEM("getproto"),
    TEST_MODULE_ITEM("i2l"),
    TEST_MODULE_ITEM("initclk"),
    TEST_MODULE_ITEM("inrval"),
    TEST_MODULE_ITEM("instrumt"),
    TEST_MODULE_ITEM("intrio"),
    TEST_MODULE_ITEM("intrupt"),
    TEST_MODULE_ITEM("io_timeout"),
    TEST_MODULE_ITEM("ioconthr"),
    TEST_MODULE_ITEM("join"),
    TEST_MODULE_ITEM("joinkk"),
    TEST_MODULE_ITEM("joinku"),
    TEST_MODULE_ITEM("joinuk"),
    TEST_MODULE_ITEM("joinuu"),
    TEST_MODULE_ITEM("layer"),
    TEST_MODULE_ITEM("lazyinit"),
    TEST_MODULE_ITEM("libfilename"),
    TEST_MODULE_ITEM("lltest"),
    TEST_MODULE_ITEM("lock"),
    TEST_MODULE_ITEM("lockfile"),
    TEST_MODULE_ITEM("logger"),
    TEST_MODULE_ITEM("many_cv"),
    TEST_MODULE_ITEM("multiwait"),
    TEST_MODULE_ITEM("nameshm1"),
    TEST_MODULE_ITEM("nblayer"),
    TEST_MODULE_ITEM("nonblock"),
    TEST_MODULE_ITEM("ntioto"),
    TEST_MODULE_ITEM("ntoh"),
    TEST_MODULE_ITEM("op_2long"),
    TEST_MODULE_ITEM("op_excl"),
    TEST_MODULE_ITEM("op_filnf"),
    TEST_MODULE_ITEM("op_filok"),
    TEST_MODULE_ITEM("op_nofil"),
    TEST_MODULE_ITEM("parent"),
    TEST_MODULE_ITEM("peek"),
    TEST_MODULE_ITEM("perf"),
    TEST_MODULE_ITEM("pipeping"),
    TEST_MODULE_ITEM("pipeping2"),
    TEST_MODULE_ITEM("pipeself"),
    TEST_MODULE_ITEM("poll_nm"),
    TEST_MODULE_ITEM("poll_to"),
    TEST_MODULE_ITEM("pollable"),
    TEST_MODULE_ITEM("prftest"),
    TEST_MODULE_ITEM("primblok"),
    TEST_MODULE_ITEM("provider"),
    TEST_MODULE_ITEM("prpollml"),
    TEST_MODULE_ITEM("ranfile"),
    TEST_MODULE_ITEM("randseed"),
    TEST_MODULE_ITEM("rwlocktest"),
    TEST_MODULE_ITEM("sel_spd"),
    TEST_MODULE_ITEM("selct_er"),
    TEST_MODULE_ITEM("selct_nm"),
    TEST_MODULE_ITEM("selct_to"),
    TEST_MODULE_ITEM("selintr"),
    TEST_MODULE_ITEM("sema"),
    TEST_MODULE_ITEM("semaerr"),
    TEST_MODULE_ITEM("semaping"),
    TEST_MODULE_ITEM("sendzlf"),
    TEST_MODULE_ITEM("server_test"),
    TEST_MODULE_ITEM("servr_kk"),
    TEST_MODULE_ITEM("servr_uk"),
    TEST_MODULE_ITEM("servr_ku"),
    TEST_MODULE_ITEM("servr_uu"),
    TEST_MODULE_ITEM("short_thread"),
    TEST_MODULE_ITEM("sigpipe"),
    TEST_MODULE_ITEM("socket"),
    TEST_MODULE_ITEM("sockopt"),
    TEST_MODULE_ITEM("sockping"),
    TEST_MODULE_ITEM("sprintf"),
    TEST_MODULE_ITEM("stack"),
    TEST_MODULE_ITEM("stdio"),
    TEST_MODULE_ITEM("str2addr"),
    TEST_MODULE_ITEM("strod"),
    TEST_MODULE_ITEM("switch"),
    TEST_MODULE_ITEM("system"),
    TEST_MODULE_ITEM("testbit"),
    TEST_MODULE_ITEM("testfile"),
    TEST_MODULE_ITEM("threads"),
    TEST_MODULE_ITEM("timemac"),
    TEST_MODULE_ITEM("timetest"),
    TEST_MODULE_ITEM("tpd"),
    TEST_MODULE_ITEM("udpsrv"),
    TEST_MODULE_ITEM("vercheck"),
    TEST_MODULE_ITEM("version"),
    TEST_MODULE_ITEM("writev"),
    TEST_MODULE_ITEM("xnotify"),
    TEST_MODULE_ITEM("zerolen"),
    END_POINT_MARKER
};


int main(int argc, char **argv)
{
    int  i,j;
    char mydir[MAX_ARG_LENGTH];

    char **pp = argv;

    



    SetLastError(0);

    TestModuleItemName *p = test_module_list;

    int skipped = 0;

    printf("\nNSPR Test Results\n");

    WCHAR dateWStr[100];
    GetDateFormat(NULL, NULL, NULL, TEXT("ddd, MMM dd yyyy"), dateWStr, 100);

    WCHAR timeWStr[100];
    GetTimeFormat(NULL, TIME_NOSECONDS, NULL, TEXT(" - h:mm tt"), timeWStr, 100);

    wcscat(dateWStr, timeWStr);

    char dateStr[100];
    int rv = WideCharToMultiByte(CP_ACP, NULL, dateWStr, 100, dateStr, 100, NULL, NULL);

    printf("BEGIN\t%s\n", dateStr);
    OutputDebugString(L"BEGINNING TEST RUN\n");

    if ( 0 < wcslen(skip_until) )
    {
        
        while ( (0 < wcslen(skip_until)) && 
			    (NULL != p) && (NULL != p->pW) && (0 < wcslen(p->pW)) && 
				  (0 != wcscmp(p->pW, skip_until)) ) 
		    {
            p++, skipped++;
        }

        if ( (NULL == p) || (NULL == p->pW) || (0 == wcslen(p->pW)) ) {
            printf("main: EXITING - no tests to run (Skipped %d tests)\n", skipped);
            return 0;
        }
    }

    printf("    Test\tResult\n\n");

    int succeeded = 0;
    int failed = 0;
    int total = 0;

    while ( (NULL != p) && (NULL != p->pW) && (0 < wcslen(p->pW)) )
    {
        int     rv      = -1;
        BOOL    bFailed = FALSE;
        HMODULE hDll    = NULL;
        WCHAR   wStr[500];

        printf("\nBEGIN TEST %s\n\n", ((NULL != p) ? p->pC : "????"));  
        wsprintf(wStr, L"BEGIN TEST - %s\n", p->pW);
        OutputDebugString(wStr);

        __try
        {
            WCHAR dllName[256];

            wsprintf(dllName, TEXT("%s.dll"), ((NULL != p) ? p->pW : TEXT("BOGUS_MODULE_NAME")));

            
            hDll = LoadLibrary(dllName);

            if ( NULL != hDll )
            {
                
                pMainFnc pMF = (pMainFnc) GetProcAddress( hDll, "nspr_test_runme" );

                if ( pMF )
                    rv = (*pMF)(0, NULL);
            }

            
            if (0 != rv) {
                failed++;
                bFailed = TRUE;
            } else
                succeeded++;
        }
        __except ( 1 )  
        {
            failed++;
            bFailed = TRUE;
        }

        if ( NULL != hDll ) {
            FreeLibrary( hDll );
            hDll = NULL;
        }

        total++;

        printf("    %s\t%s\n", ((NULL != p) ? p->pC : "????"), (bFailed ? "FAILED" : "Passed"));

		{
			WCHAR str[400];
			if ( bFailed )
				wsprintf(str, TEXT("----FAILED----: TEST %s: ------FAILED------\n"), p->pW);
			else
				wsprintf(str, TEXT("++++PASSED++++: TEST %s: ++++++PASSED++++++\n"), p->pW);
			OutputDebugString(str);
		}

        printf("\nEND TEST %s\n\n", ((NULL != p) ? p->pC : "????"));    

        p++;
    }

    printf("\n\n**********\nNSPR Test Results:");
    printf("    Skipped Tests = %d", skipped);
    printf("    Total Run Tests = %d", total);
    printf("    Succeeded Tests = %d", succeeded);
    printf("    Failed Tests = %d\n\n**********\n", failed);

    return 0;
}

#else 

int main(int argc, char **argv)
{
    printf( "This application is not supported outside of WinCE/WinMobile\n" );
    return 0;
}

#endif 
