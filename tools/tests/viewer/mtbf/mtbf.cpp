
























































#include <iostream.h>
#include <fstream.h>
#include <process.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>

const char* gStatus[]={"abnormally","normally"};

bool gSendMail=false;
const char* gDefDir="s:";
char  gMTBFDirectory[500]="";
char  gDirectory[10];
const char* gDefProgram="viewer.exe";
char  gProgram[500];

char  gSubject[]="\"Automated MTBF Report\"";
char  gRecipient[500]="rgess@san.rr.com";

char  gReportFilename[20]="report.html";
char  gReportPath[500]="";

const char* gURLFilename="urls.txt";
char  gURLPath[500];

int gLoop = 100;

void ExitWithError(const char* aMsg1,const char* aMsg2){
  cout << endl << "Error: " << aMsg1;
  if(aMsg2)
    cout<< " " << aMsg2;
  cout <<"."<< endl;
  cout << "Process terminated." << endl;
  exit(2000);
}








void ComputeEXEDirectory(char* aProgramPath,char* anOutputFilename){
  char* thePtr=strrchr(aProgramPath,'\\');
  if(thePtr) {
    strcpy(anOutputFilename,aProgramPath);
    anOutputFilename[thePtr-aProgramPath+1]=0;
  }
  thePtr=strchr(anOutputFilename,'\\');
  while(thePtr) {
    *thePtr='/';
    thePtr=strchr(anOutputFilename,'\\');
  }
}







void initializeSettings(int argc,char* argv[]){
  bool theURLFileIsValid=false;
  ComputeEXEDirectory(argv[0],gMTBFDirectory);
  sprintf(gReportPath,"%s%s",gMTBFDirectory,gReportFilename);
  sprintf(gURLPath,"%s%s",gMTBFDirectory,gURLFilename);

  for(int i=0;i<argc;i++){
    if(0==strnicmp(argv[i],"-f",2)){
      
      strcpy(gURLPath, argv[i+1]);
      fstream in(gURLPath,ios::in);
      char buf[500];
      buf[0]=0;
      in>>buf;
      if(!buf[0]){
        ExitWithError("Unable to read the given URL file",gURLPath);
      }
      else {
	theURLFileIsValid=true;
      }
      
    }
    else if(0==strnicmp(argv[i],"-r",2)){
      
      strcpy(gRecipient,argv[i+1]);
      gSendMail=true;
    }
    else if(0==strnicmp(argv[i],"-l",2)){
      gLoop = atoi(argv[i+1]);
      if ((gLoop > 200) || (gLoop < 0))
        ExitWithError("Loop must be a positive value and less than 200", 0);
    }
  }
  if(!theURLFileIsValid)
    ExitWithError("You must provide a valid URL textfile",0);
}







void PrintResultsAsHTML(ostream& aStream,time_t& aStart,time_t& aEnd,int aResult){
  time_t ltime;
  time( &ltime );

  







  int total=aEnd-aStart;
  int mins=(aEnd-aStart)/60;
  char buffer[50];
  sprintf(buffer,"%i mins %i sec",mins,total%60);

  aStream << "Automated MTBF Report"<< endl;
  aStream <<  ctime(&ltime) << endl;
  aStream << "Running: "<< gDirectory << gProgram << endl;
  aStream << "Elapsed: "<< buffer << endl;
  aStream << "Exit: "<< gStatus[0==aResult] << endl;


}








void PrintResults(ostream& aStream,time_t& aStart,time_t& aEnd,int aResult){
  time_t ltime;
  time( &ltime );

  aStream << "=========================================" << endl;
  aStream << "Gecko \"Mean Time Between Failure\" Report" << endl;
  aStream << "=========================================" << endl;
  aStream << "Date:    " << ctime( &ltime);
  aStream << "Running: " << gDirectory << gProgram << endl;
  aStream << "File:    " << gURLPath << endl;

  int total=aEnd-aStart;
  int mins=(aEnd-aStart)/60;
  aStream << "MTBF:    ";
    if(mins>0){
    aStream << mins << " min ";
  }
  const char* status[]={"abnormally","normally"};
  aStream << total%60 << " seconds" << endl;

  aStream << "Exit:  Program terminated " << status[0==aResult] << endl;
}








bool getPath(char* aBuffer,const char* aFilename,int index){
  bool result=false;
  if(aFilename){
    int len=strlen(aFilename);
    if(len>0){
      int hit=0;
      int pos=len;
      while((hit<index) && (--pos>-1)){
        if((aFilename[pos]=='\\') || (aFilename[pos]=='/')){
          hit++;
        }
      }
      if(hit==index){
        strncpy(aBuffer,aFilename,pos);
        aBuffer[pos]=0;
        strcat(aBuffer,"\\cache");
        result=true;
      }
    }
  }
  return result;
}







int main(int argc,char* argv[]) {

  int result=0;

  

  if(argc>1) {









    strcpy(gProgram,gDefProgram);
    initializeSettings(argc,argv);

    
    time_t startTime;  
    time( &startTime ) ;  

    char buffer[500];
    sprintf(buffer,"%s%s -f %s -r %d",gDirectory,gProgram,gURLPath,gLoop);
    result=system(buffer);

    time_t endTime;  
    time( &endTime ) ;  

    if(gSendMail) {
      {
        
        fstream out(gReportPath,ios::out);
        PrintResultsAsHTML(out,startTime,endTime,result);
      }
      sprintf(buffer,"blat %s -s %s -t %s",gReportPath,gSubject,gRecipient);
      result=system(buffer);
    }
    else {
      cout << endl << endl;
      PrintResults(cout,startTime,endTime,result);
    }

    int err=errno;
    switch(errno){
      case E2BIG:
        cout << "too big" << endl; break;
      case EACCES:
        cout << "access" << endl; break;
      case EMFILE:
        cout << "file" << endl; break;
      case ENOENT:
        cout << "bad file or directory" << endl; break;
      case ENOEXEC:
        cout << "exec" << endl; break;
      case ENOMEM:
        cout << "no mem" << endl; break;
      default:
        break;
    }
  }
  else {
    printf("MTBF (Mean time between failures) version 1.1.\n");
    printf("This program requires BLAT.EXE, which you can find on the net.\n");
    printf("http://gepasi.dbs.aber.ac.uk/softw/Blat.html \n");
    printf("Optional arguments: \n");
    printf("  -f filename containing URL list\n");
    printf("  -r email recipient\n");
    printf("  -l loop # of times (default is 100, max is 200)\n");
  }
  return 0;
}
