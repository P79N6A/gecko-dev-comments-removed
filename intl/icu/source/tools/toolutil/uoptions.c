

















#include "unicode/utypes.h"
#include "cstring.h"
#include "uoptions.h"

U_CAPI int U_EXPORT2
u_parseArgs(int argc, char* argv[],
            int optionCount, UOption options[]) {
    char *arg;
    int i=1, remaining=1;
    char c, stopOptions=0;

    while(i<argc) {
        arg=argv[i];
        if(!stopOptions && *arg=='-' && (c=arg[1])!=0) {
            
            UOption *option=NULL;
            arg+=2;
            if(c=='-') {
                
                if(*arg==0) {
                    
                    stopOptions=1;
                } else {
                    
                    int j;
                    for(j=0; j<optionCount; ++j) {
                        if(options[j].longName && uprv_strcmp(arg, options[j].longName)==0) {
                            option=options+j;
                            break;
                        }
                    }
                    if(option==NULL) {
                        
                        return -i;
                    }
                    option->doesOccur=1;

                    if(option->hasArg!=UOPT_NO_ARG) {
                        
                        if(i+1<argc && !(argv[i+1][0]=='-' && argv[i+1][1]!=0)) {
                            
                            option->value=argv[++i];
                        } else if(option->hasArg==UOPT_REQUIRES_ARG) {
                            
                            return -i;
                        }
                    }
                }
            } else {
                
                do {
                    
                    int j;
                    for(j=0; j<optionCount; ++j) {
                        if(c==options[j].shortName) {
                            option=options+j;
                            break;
                        }
                    }
                    if(option==NULL) {
                        
                        return -i;
                    }
                    option->doesOccur=1;

                    if(option->hasArg!=UOPT_NO_ARG) {
                        
                        if(*arg!=0) {
                            
                            option->value=arg;
                            
                            break;
                        } else if(i+1<argc && !(argv[i+1][0]=='-' && argv[i+1][1]!=0)) {
                            
                            option->value=argv[++i];
                            
                            break;
                        } else if(option->hasArg==UOPT_REQUIRES_ARG) {
                            
                            return -i;
                        }
                    }

                    
                    option=NULL;
                    c=*arg++;
                } while(c!=0);
            }

            if(option!=0 && option->optionFn!=0 && option->optionFn(option->context, option)<0) {
                
                return -i;
            }

            
            ++i;
        } else {
            
            argv[remaining++]=arg;
            ++i;
        }
    }
    return remaining;
}
