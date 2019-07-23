



































#include "OJITestLoader.h"

#include "nsReadableUtils.h"

static NS_DEFINE_IID(kISupportsIID,    NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIOJITestLoaderIID, OJITESTLOADER_IID);

NS_IMPL_ISUPPORTS1(OJITestLoader, OJITestLoader)

OJITestLoader::OJITestLoader(void) 
{

  TestResult* res = NULL;
  char** testCase = loadTestList();
  int i = 0;
  TEST_TYPE testType;

  if (!testCase) {
	fprintf(stderr, "ERROR: Can't load test list !\n");
	return;
  }
  if (!(fdResFile = PR_Open(OJI_TEST_RESULTS, PR_CREATE_FILE | PR_WRONLY, PR_IRWXU))) {
    fprintf(stderr, "ERROR: Can't open test results file !\n");
  }
  for(i=0; testCase[i]; i++) {
    res = NULL;
    switch(testType = getTestType(testCase[i])) {
    case(JNI):
      res = runTest(testCase[i], OJI_JNI_TESTS);
      break;
    case(JM):
      res = runTest(testCase[i], OJI_JM_TESTS);
      break;
    case(TM):
      res = runTest(testCase[i], OJI_TM_TESTS);
      break;
    case(LCM):
      res = runTest(testCase[i], OJI_LCM_TESTS);
      break;
    case(JMPTI):
      res = runTest(testCase[i], OJI_JMPTI_TESTS);
      break;
    default:
      fprintf(stderr, "Can't determine test type (%s)\n", testCase[i]);
    }  
    if (res)
      registerRes(res, testCase[i]);
    else
      registerRes(TestResult::FAIL("Test execution failed"), testCase[i]);
  }


}


TestResult* OJITestLoader::runTest(const char* testCase, const char* libName) {

  PRLibrary* lib = PR_LoadLibrary(libName);
  OJI_TESTPROC testProc = NULL; 

  if (lib) {
	testProc = (OJI_TESTPROC)PR_FindSymbol(lib, testCase);
	if (testProc) {
		return testProc();
	} else {
		fprintf(stderr, "WARNING: Can't find %s method in %s library\n", testCase, libName);
	}
  } else {
	fprintf(stderr, "WARNING: Can't load %s library\n", libName);
  }
  return NULL;
}


void OJITestLoader::registerRes(TestResult* res, char* tc){
	char *outBuf = (char*)calloc(1, res->comment.Length() + PL_strlen(tc) + 100);
	
	sprintf(outBuf, "%s: %s (%s)\n", tc, res->status?"PASS":"FAILED", NS_LossyConvertUTF16toASCII(res->comment).get());
	if (fdResFile) {	
		printf("%s", outBuf);	
		if (PR_Write(fdResFile, outBuf, PL_strlen(outBuf)) < PL_strlen(outBuf))
			fprintf(stderr, "WARNING: Can't write entire result message. Possibly not enough free disk space !\n");
	} else {
		printf("%s", outBuf);
	}
	free(outBuf);
}


OJITestLoader::~OJITestLoader() 
{
  if(fdResFile)
    PR_Close(fdResFile);
}


char** OJITestLoader::loadTestList() {
	struct stat st;
	FILE *file = NULL;
	char *content;
	int nRead, nTests, count = 0;
	char *pos, *pos1;
	char **testList;

	if (stat(OJI_TESTS_LIST, &st) < 0) {
		fprintf(stderr, "ERROR: can't get stat from file %s\n", OJI_TESTS_LIST);
		return NULL;
	}
	content = (char*)calloc(1, st.st_size+1);
	if ((file = fopen(OJI_TESTS_LIST, "r")) == NULL) {
		fprintf(stderr, "ERROR: can't open file %s\n", OJI_TESTS_LIST);
		return NULL;
	}
	if ((nRead = fread(content, 1, st.st_size, file)) < st.st_size) {
		fprintf(stderr, "WARNING: can't read entire file in text mode (%d of %d) !\n", nRead, st.st_size);
		
	}
	content[nRead] = 0;
	
	fclose(file);

	
	nTests = countChars(content, '\n') + 2;
	printf("nTests = %d\n", nTests);
	testList = (char**)calloc(sizeof(char*), nTests);
	testList[0] = 0;
	pos = content;
	while((pos1 = PL_strchr(pos, '\n'))) {
		*pos1 = 0;
		if(*pos && *pos != '#') {
		  
			testList[count++] = PL_strdup(pos);
		}
		pos = pos1+1;
		if (!(*pos)) {
			printf("Parser done: %d .. ", count);
			testList[count] = 0;
			printf("ok\n");
			break;
		}
	}
	
	if (*pos && *pos != '#') {
		testList[count++] = PL_strdup(pos);
		testList[count] = 0;
	}
	
	return testList;
	
}


int OJITestLoader::countChars(char* buf, char ch) {
	char *pos = buf;
	int count = 0;

	while((pos = PL_strchr(pos, ch))) { 
		pos++;
		count++;
	}
	return count;
} 



TEST_TYPE OJITestLoader::getTestType(char *tc) {
	char *buf = PL_strdup(tc);
	char *pos = PL_strchr(buf, '_');
	if (!pos)
		return (TEST_TYPE)-1;
	*pos = 0;
	for(int i=0; test_types[i]; i++) {
		if (!PL_strcmp(buf, test_types[i])) {
			free(buf);
			return (TEST_TYPE)i;
		}
	}
	free(buf);
	return (TEST_TYPE)-1;
}

NS_METHOD OJITestLoader::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr) {
	OJITestLoader *tl = new OJITestLoader();
	if (!aInstancePtr)
		return NS_ERROR_NULL_POINTER;
	*aInstancePtr = nsnull;
	if (aIID.Equals(kISupportsIID)) 
		*aInstancePtr  = (void*)(nsISupports*)tl;
	if (aIID.Equals(kIOJITestLoaderIID))
		*aInstancePtr = (void*)tl;
	if(!(*aInstancePtr))
		return NS_ERROR_NO_INTERFACE;
	return NS_OK;
}
