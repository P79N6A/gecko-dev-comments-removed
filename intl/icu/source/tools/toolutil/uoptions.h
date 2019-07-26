

















#ifndef __UOPTIONS_H__
#define __UOPTIONS_H__

#include "unicode/utypes.h"



    


#if defined(XP_MAC_CONSOLE)
#   include <console.h>
    
#   define U_MAIN_INIT_ARGS(argc, argv) argc = ccommand((char***)&argv)
#else
    
#   define U_MAIN_INIT_ARGS(argc, argv)
#endif




struct UOption;
typedef struct UOption UOption;


typedef int UOptionFn(void *context, UOption *option);


enum { UOPT_NO_ARG, UOPT_REQUIRES_ARG, UOPT_OPTIONAL_ARG };


struct UOption {
    const char *longName;   
    const char *value;      
    UOptionFn *optionFn;    
    void *context;          
    char shortName;         
    char hasArg;            
    char doesOccur;         
};


#define UOPTION_DEF(longName, shortName, hasArg) \
    { longName, NULL, NULL, NULL, shortName, hasArg, 0 }


#define UOPTION_HELP_H              UOPTION_DEF("help", 'h', UOPT_NO_ARG)
#define UOPTION_HELP_QUESTION_MARK  UOPTION_DEF("help", '?', UOPT_NO_ARG)
#define UOPTION_VERBOSE             UOPTION_DEF("verbose", 'v', UOPT_NO_ARG)
#define UOPTION_QUIET               UOPTION_DEF("quiet", 'q', UOPT_NO_ARG)
#define UOPTION_VERSION             UOPTION_DEF("version", 'V', UOPT_NO_ARG)
#define UOPTION_COPYRIGHT           UOPTION_DEF("copyright", 'c', UOPT_NO_ARG)

#define UOPTION_DESTDIR             UOPTION_DEF("destdir", 'd', UOPT_REQUIRES_ARG)
#define UOPTION_SOURCEDIR           UOPTION_DEF("sourcedir", 's', UOPT_REQUIRES_ARG)
#define UOPTION_ENCODING            UOPTION_DEF("encoding", 'e', UOPT_REQUIRES_ARG)
#define UOPTION_ICUDATADIR          UOPTION_DEF("icudatadir", 'i', UOPT_REQUIRES_ARG)
#define UOPTION_WRITE_JAVA          UOPTION_DEF("write-java", 'j', UOPT_OPTIONAL_ARG)
#define UOPTION_PACKAGE_NAME        UOPTION_DEF("package-name", 'p', UOPT_REQUIRES_ARG)
#define UOPTION_BUNDLE_NAME         UOPTION_DEF("bundle-name", 'b', UOPT_REQUIRES_ARG)

























































U_CAPI int U_EXPORT2
u_parseArgs(int argc, char* argv[],
            int optionCount, UOption options[]);

#endif
