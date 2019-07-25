




































#include "nsXPCOM.h"
#include "nsIComponentManager.h"
#include "nsParserCIID.h"
#include "nsIAtom.h"
#include "nsIParser.h"
#include "nsILoggingSink.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsIURI.h"
#include "CNavDTD.h"
#include <fstream.h>


static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);
static NS_DEFINE_IID(kLoggingSinkCID, NS_LOGGING_SINK_CID);





static const char* kWorkingDir = "./";

nsresult GenerateBaselineFile(const char* aSourceFilename,const char* aBaselineFilename)
{
  if (!aSourceFilename || !aBaselineFilename)
     return NS_ERROR_INVALID_ARG;

  nsresult rv;

  
  nsCOMPtr<nsIParser> parser(do_CreateInstance(kParserCID, &rv));
  if (NS_FAILED(rv)) {
    cout << "Unable to create a parser (" << rv << ")" <<endl;
    return rv;
  }

  
  nsCOMPtr<nsILoggingSink> sink(do_CreateInstance(kLoggingSinkCID, &rv));
  if (NS_FAILED(rv)) {
    cout << "Unable to create a sink (" << rv << ")" <<endl;
    return rv;
  }

  nsCOMPtr<nsILocalFile> localfile (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  localfile->InitWithNativePath(nsDependentCString(aSourceFilename));
  nsCOMPtr<nsIURI> inputURI;
  {
    nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = ioService->NewFileURI(localfile, getter_AddRefs(inputURI));
    if (NS_FAILED(rv))
      return rv;
  }
  localfile->InitWithNativePath(nsDependentCString(aBaselineFilename));
  PRFileDesc *outputfile;
  localfile->OpenNSPRFileDesc(0660, PR_WRONLY | PR_CREATE_FILE, &outputfile);
  sink->SetOutputStream(outputfile);

  
  parser->SetContentSink(sink);

  rv = parser->Parse(inputURI, 0, PR_FALSE, eDTDMode_unknown);

  return rv;
}



PRBool CompareFiles(const char* aFilename1, const char* aFilename2) {
  PRBool result=PR_TRUE;

  fstream theFirstStream(aFilename1,ios::in | ios::nocreate);
  fstream theSecondStream(aFilename2,ios::in | ios::nocreate);

  PRBool done=PR_FALSE;
  char   ch1,ch2;

  while(!done) {
    theFirstStream >> ch1;
    theSecondStream >> ch2;
    if(ch1!=ch2) {
      result=PR_FALSE;
      break;
    }
    done=PRBool((theFirstStream.ipfx(1)==0) || (theSecondStream.ipfx(1)==0));
  }
  return result;
}



void ComputeTempFilename(const char* anIndexFilename, char* aTempFilename) {
  if(anIndexFilename) {
    strcpy(aTempFilename,anIndexFilename);
    char* pos=strrchr(aTempFilename,'\\');
    if(!pos)
      pos=strrchr(aTempFilename,'/');
    if(pos) {
      (*pos)=0;
      strcat(aTempFilename,"/temp.blx");
      return;
    }
  }
  
  strcpy(aTempFilename,"c:/windows/temp/temp.blx");
}



static const char* kAppName = "logparse ";
static const char* kOption1 = "Compare baseline file-set";
static const char* kOption2 = "Generate baseline ";
static const char* kResultMsg[2] = {" failed!"," ok."};

void ValidateBaselineFiles(const char* anIndexFilename) {

  fstream theIndexFile(anIndexFilename,ios::in | ios::nocreate);
  char    theFilename[500];
  char    theBaselineFilename[500];
  char    theTempFilename[500];
  PRBool  done=PR_FALSE;

  ComputeTempFilename(anIndexFilename,theTempFilename);

  while(!done) {
    theIndexFile >> theFilename;
    theIndexFile >> theBaselineFilename;
    if(theFilename[0] && theBaselineFilename[0]) {
      if(NS_SUCCEEDED(GenerateBaselineFile(theFilename,theTempFilename))) {
        PRBool matches=CompareFiles(theTempFilename,theBaselineFilename);
        cout << theFilename << kResultMsg[matches] << endl;
      }
    }
    theFilename[0]=0;
    theBaselineFilename[0]=0;
    done=PRBool(theIndexFile.ipfx(1)==0);
  }


  




}




int main(int argc, char** argv)
{
  if (argc < 2) {
    cout << "Usage: " << kAppName << " [options] [filename]" << endl;
    cout << "     -c [filelist]   " << kOption1 << endl;
    cout << "     -g [in] [out]   " << kOption2 << endl;
    return -1;
  }

  int result=0;

  nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
  if (NS_FAILED(rv)) {
    printf("NS_InitXPCOM2 failed\n");
    return 1;
  }

  if(0==strcmp("-c",argv[1])) {

    if(argc>2) {
      cout << kOption1 << "..." << endl;

      
      
      ValidateBaselineFiles(argv[2]);
    }
    else {
      cout << kAppName << ": Filelist missing for -c option -- nothing to do." << endl;
    }

  }
  else if(0==strcmp("-g",argv[1])) {
    if(argc>3) {
      cout << kOption2 << argv[3] << " from " << argv[2] << "..." << endl;
      GenerateBaselineFile(argv[2],argv[3]);
    }
    else {
      cout << kAppName << ": Filename(s) missing for -g option -- nothing to do." << endl;
    }
  }
  else {
    cout << kAppName << ": Unknown options -- nothing to do." << endl;
  }
  return result;
}
