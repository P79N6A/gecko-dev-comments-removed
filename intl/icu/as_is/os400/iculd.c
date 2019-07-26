


















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef TEST_MODE
#define TEST_MODE 0
#endif


#if !TEST_MODE
#include <qp0z1170.h>
#else
static int Qp0zSystem(const char *cmd) {
  printf("CL: %s\n", cmd);
  return 0;
}
#endif

static int runcmd(const char *cmd) {
  int rc;
  printf("%s\n", cmd);
  rc = Qp0zSystem(cmd);
  if(rc==0) {
    printf("..ok\n");
    return 0;
  } else if(rc<0){
    printf("..Qp0zSystem failed.\n");
    return 1;
  } else {
    printf("..System call failed.\n");
    return 1;
  }
}

int main(int argc, const char *argv[]) {
  int i;

  char buf[8048];
  char opt[4100];
  char objs[4024];
  char libs[4024];
  char bnddirs[4024];
  const char *prog="";
  const char *progshort=prog;
  const char *outputdir=getenv("OUTPUTDIR");

  printf("# OUTPUTDIR=%s ",outputdir);
  for(i=0;i<argc;i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");

  buf[0]=0;
  opt[0]=0;
  objs[0]=0;
  libs[0]=0;
  bnddirs[0]=0;

  for(i=1;i<argc;i++) {
    if(argv[i][0]=='-') {
      switch(argv[i][1]) {
      case 'O':
        printf(".. ignoring optimization: %s\n", argv[i]);
        break;
      case 'g':
        printf(".. ignoring debugging: %s\n", argv[i]);
        break;
      case 'l':
        printf(".. ignoring lib: %s\n", argv[i]);
        break;
      case 'v':
        printf(".. already verbose\n");
        break;
      case 'o':
        i++;
        prog=argv[i];
        progshort=strrchr(prog,'/');
        if(!progshort) {
          progshort=prog;
        } else {
          progshort++; 
        }
        break;
      case 'q':
        if(!strncmp(argv[i]+2,"OPTION=",7)) {
          strcat(opt,argv[i]+9);
        } else {
          printf("Unknown -q option: %s\n", argv[i]);
          return 1;
        }
        break;
      default:
        printf("Unknown option: %s\n", argv[i]);
        return 1;
      }
    } else {
      int n = strlen(argv[i]);
      if(argv[i][n-1]=='o' &&
         argv[i][n-2]=='.') {
        const char *b = argv[i];
        char linkbuf[200];
        char outbuf[100];
        int nlen = n-2;

        if(nlen >= 10) {
          nlen = 10;
        }

        if(readlink(b,linkbuf,200)>0) {
          
          
          char *mend = strrchr(linkbuf,'.');  
          if(mend) {
            *mend=0;
            mend = strrchr(linkbuf,'/');
            if(mend) {
              mend++;
              strcpy(outbuf,mend);
              b=outbuf;
              nlen=strlen(b);
            }
          }
        } else {
          

        }

        strcat(objs,outputdir);
        strcat(objs,"/");
        strncat(objs,b,nlen);
        strcat(objs, " ");
      } else if(argv[i][n-1]=='a' &&
         argv[i][n-2]=='.') {
        const char *b = argv[i];
        char linkbuf[200];
        char outbuf[100];
        int nlen = n-2;

        if(nlen >= 10) {
          nlen = 10;
        }

        if(readlink(b,linkbuf,200)>0) {
          
          
          char *mend = strrchr(linkbuf,'.');  
          if(mend) {
            *mend=0;
            mend = strrchr(linkbuf,'/');
            if(mend) {
              mend++;
              strcpy(outbuf,mend);
              b=outbuf;
              nlen=strlen(b);
            }
          }
        } else {
          

        }

        strcat(bnddirs,outputdir);
        strcat(bnddirs,"/");
        strncat(bnddirs,b,nlen);
        strcat(bnddirs, " ");
      } else if(argv[i][n-1]=='o' &&
         argv[i][n-2]=='s' &&
         argv[i][n-3]=='.') {
        const char *p = strrchr(argv[i],'/');
        if(!p) {
          printf("Can't find trailing slash in %s\n", argv[i]);
          return 1;
        }
        strcat(libs,outputdir);
        strcat(libs,"/");
        strncat(libs,p+1,strlen(p)-4);
        strcat(libs," ");
      } else {
        printf("Unknown input file: %s\n", argv[i]);
        return 1;
      }
    }
  }

  if(prog[0]==0) {
    printf("no program (-o) option specified.\n");
    return 1;
  }

  sprintf(buf,"CRTPGM PGM(%s/%s) MODULE(%s) BNDSRVPGM(%s) BNDDIR(%s) OPTION(%s) REPLACE(*YES)",
          outputdir,progshort,

          objs,

          libs,

          bnddirs,

          opt);


  if(runcmd(buf)) {
    return 1;
  }

  
  {
    char path1[1000];
    sprintf(path1,"/qsys.lib/%s.lib/%s.pgm",
            outputdir,
            progshort);
    printf("# ln -s %s %s\n", path1, prog);
    if((!TEST_MODE) && symlink(path1,prog)) {
      perror("symlink");
      if(errno!=EEXIST) { 
        return 1;
      }
    }
  }
  return 0;
}








