



































#include <stdio.h>
#include <sys\stat.h>

int main(int iArgc, char **ppArgv) {
    int iRetval = 1;
    
    


    char *pFileName = ppArgv[1];
    if(pFileName) {
        FILE *pFile = fopen(pFileName, "rb");
        if(pFile) {
            struct stat sInfo;
            
            

            if(!fstat(fileno(pFile), &sInfo)) {
                int iChar;
                int iX = 0;
                int iFirsttime = 1;
                
                

                printf("BEGIN\n");

                




                printf("\t\"bin2rc generated resource\\0\",\t// bin2rc identity string\n");

                





                printf("\t\"%s\\0\",\t// optional command line string\n", ppArgv[2] ? ppArgv[2] : "");
                
                



                printf("\t\"%ld\\0\"\t// data size header\n", sInfo.st_size);
                
                while(EOF != (iChar = fgetc(pFile))) {
                    

                    if(0 == iFirsttime) {
                        iX += printf(",");
                    }
                    else {
                        iFirsttime = 0;
                    }
                    
                    

                    if(iX >= 72) {
                        printf("\n");
                        iX = 0;
                    }
                    
                    

                    if(0 == iX) {
                        printf("\t");
                        iX += 8;
                    }
                    
                    

                    iX += printf("\"\\%.3o\"", iChar);
                    
                    
                }
                
                

                if(0 != iX) {
                    printf("\n");
                }
                printf("END\n");
                
                

                iRetval = 0;
            }
            fclose(pFile);
            pFile = NULL;
        }
    }
    
    return(iRetval);
}

