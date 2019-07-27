





#if defined(hpux)
# ifndef _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_POSIX_SOURCE
# endif
#endif

#include "simplethread.h"

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "uparse.h"
#include "unicode/localpointer.h"
#include "unicode/resbund.h"
#include "unicode/udata.h"
#include "unicode/uloc.h"
#include "unicode/locid.h"
#include "putilimp.h"
#include "intltest.h"
#include "tsmthred.h"
#include "unicode/ushape.h"
#include "unicode/translit.h"
#include "sharedobject.h"
#include "unifiedcache.h"
#include "uassert.h"

#if U_PLATFORM_USES_ONLY_WIN32_API
    
#   undef POSIX
#elif U_PLATFORM_IMPLEMENTS_POSIX
#   define POSIX
#else
#   undef POSIX
#endif


#if U_PLATFORM == U_PF_OS390
#define __DOT1 1
#define __UU
#ifndef _XPG4_2
#define _XPG4_2
#endif
#include <unistd.h>
#endif
#if defined(POSIX)

#define HAVE_IMP

#if (ICU_USE_THREADS == 1)
#include <pthread.h>
#endif

#if defined(__hpux) && defined(HPUX_CMA)
# if defined(read)  
#  undef read
# endif
#endif


#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif

#if U_PLATFORM == U_PF_OS390
#include <sys/types.h>
#endif

#if U_PLATFORM != U_PF_OS390
#include <signal.h>
#endif


#ifndef _XPG4_2
#define _XPG4_2
#endif


#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif


#ifndef _INCLUDE_XOPEN_SOURCE_EXTENDED
#define _INCLUDE_XOPEN_SOURCE_EXTENDED
#endif

#include <unistd.h>

#endif

#ifdef sleep
#undef sleep
#endif

#define TSMTHREAD_FAIL(msg) errln("%s at file %s, line %d", msg, __FILE__, __LINE__)
#define TSMTHREAD_ASSERT(expr) {if (!(expr)) {TSMTHREAD_FAIL("Fail");}}

MultithreadTest::MultithreadTest()
{
}

MultithreadTest::~MultithreadTest()
{
}



#if (ICU_USE_THREADS==0)
void MultithreadTest::runIndexedTest( int32_t index, UBool exec,
                const char* &name, char*  ) {
  if (exec) logln("TestSuite MultithreadTest: ");

  if(index == 0)
      name = "NO_THREADED_TESTS";
  else
      name = "";

  if(exec) { logln("MultithreadTest - test DISABLED.  ICU_USE_THREADS set to 0, check your configuration if this is a problem..");
  }
}
#else

#include <stdio.h>
#include <string.h>
#include <ctype.h>    

#include "unicode/putil.h"


#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/locid.h"
#include "unicode/coll.h"
#include "unicode/calendar.h"
#include "ucaconf.h"

void SimpleThread::errorFunc() {
    
}

void MultithreadTest::runIndexedTest( int32_t index, UBool exec,
                const char* &name, char*  ) {
    if (exec)
        logln("TestSuite MultithreadTest: ");
    switch (index) {
    case 0:
        name = "TestThreads";
        if (exec)
            TestThreads();
        break;

    case 1:
        name = "TestMutex";
        if (exec)
            TestMutex();
        break;

    case 2:
        name = "TestThreadedIntl";
#if !UCONFIG_NO_FORMATTING
        if (exec) {
            TestThreadedIntl();
        }
#endif
        break;

    case 3:
      name = "TestCollators";
#if !UCONFIG_NO_COLLATION
      if (exec) {
            TestCollators();
      }
#endif 
      break;

    case 4:
        name = "TestString";
        if (exec) {
            TestString();
        }
        break;

    case 5:
        name = "TestArabicShapingThreads";
        if (exec) {
            TestArabicShapingThreads();
        }
        break;

    case 6:
        name = "TestAnyTranslit";
        if (exec) {
            TestAnyTranslit();
        }
        break;

    case 7:
        name = "TestConditionVariables";
        if (exec) {
            TestConditionVariables();
        }
        break;
    case 8:
        name = "TestUnifiedCache";
        if (exec) {
            TestUnifiedCache();
        }
        break;
    default:
        name = "";
        break; 
    }
}











#define THREADTEST_NRTHREADS 8
#define ARABICSHAPE_THREADTEST 30

class TestThreadsThread : public SimpleThread
{
public:
    TestThreadsThread(char* whatToChange) { fWhatToChange = whatToChange; }
    virtual void run() { SimpleThread::sleep(1000);
                         Mutex m;
                         *fWhatToChange = '*';
    }
private:
    char *fWhatToChange;
};










class TestArabicShapeThreads : public SimpleThread
{
public:
    TestArabicShapeThreads(char* whatToChange) { fWhatToChange = whatToChange;}
    virtual void run() {
	    if(doTailTest()==TRUE)
			*fWhatToChange = '*';
    }
private:
    char *fWhatToChange;
	
	UBool doTailTest(void) {
  static const UChar src[] = { 0x0020, 0x0633, 0 };
  static const UChar dst_old[] = { 0xFEB1, 0x200B,0 };
  static const UChar dst_new[] = { 0xFEB1, 0xFE73,0 };
  UChar dst[3] = { 0x0000, 0x0000,0 };
  int32_t length;
  UErrorCode status;
  IntlTest inteltst =  IntlTest();

  status = U_ZERO_ERROR;
  length = u_shapeArabic(src, -1, dst, UPRV_LENGTHOF(dst),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_SEEN_TWOCELL_NEAR, &status);
  if(U_FAILURE(status)) {
	   inteltst.errln("Fail: status %s\n", u_errorName(status));
	return FALSE;
  } else if(length!=2) {
    inteltst.errln("Fail: len %d expected 3\n", length);
	return FALSE;
  } else if(u_strncmp(dst,dst_old,UPRV_LENGTHOF(dst))) {
    inteltst.errln("Fail: got U+%04X U+%04X expected U+%04X U+%04X\n",
            dst[0],dst[1],dst_old[0],dst_old[1]);
	return FALSE;
  }


  
  status = U_ZERO_ERROR;
  length = u_shapeArabic(src, -1, dst, UPRV_LENGTHOF(dst),
                         U_SHAPE_LETTERS_SHAPE|U_SHAPE_SEEN_TWOCELL_NEAR|U_SHAPE_TAIL_NEW_UNICODE, &status);
  if(U_FAILURE(status)) {
    inteltst.errln("Fail: status %s\n", u_errorName(status));
	return FALSE;
  } else if(length!=2) {
    inteltst.errln("Fail: len %d expected 3\n", length);
	return FALSE;
  } else if(u_strncmp(dst,dst_new,UPRV_LENGTHOF(dst))) {
    inteltst.errln("Fail: got U+%04X U+%04X expected U+%04X U+%04X\n",
            dst[0],dst[1],dst_new[0],dst_new[1]);
	return FALSE;
  }


  return TRUE;

}
	

};

void MultithreadTest::TestThreads()
{
    char threadTestChars[THREADTEST_NRTHREADS + 1];
    SimpleThread *threads[THREADTEST_NRTHREADS];
    int32_t numThreadsStarted = 0;

    int32_t i;
    for(i=0;i<THREADTEST_NRTHREADS;i++)
    {
        threadTestChars[i] = ' ';
        threads[i] = new TestThreadsThread(&threadTestChars[i]);
    }
    threadTestChars[THREADTEST_NRTHREADS] = '\0';

    logln("->" + UnicodeString(threadTestChars) + "<- Firing off threads.. ");
    for(i=0;i<THREADTEST_NRTHREADS;i++)
    {
        if (threads[i]->start() != 0) {
            errln("Error starting thread %d", i);
        }
        else {
            numThreadsStarted++;
        }
        SimpleThread::sleep(100);
        logln(" Subthread started.");
    }

    logln("Waiting for threads to be set..");
    if (numThreadsStarted == 0) {
        errln("No threads could be started for testing!");
        return;
    }

    int32_t patience = 40; 

    while(patience--)
    {
        int32_t count = 0;
        umtx_lock(NULL);
        for(i=0;i<THREADTEST_NRTHREADS;i++)
        {
            if(threadTestChars[i] == '*')
            {
                count++;
            }
        }
        umtx_unlock(NULL);

        if(count == THREADTEST_NRTHREADS)
        {
            logln("->" + UnicodeString(threadTestChars) + "<- Got all threads! cya");
            for(i=0;i<THREADTEST_NRTHREADS;i++)
            {
                delete threads[i];
            }
            return;
        }

        logln("->" + UnicodeString(threadTestChars) + "<- Waiting..");
        SimpleThread::sleep(500);
    }

    errln("->" + UnicodeString(threadTestChars) + "<- PATIENCE EXCEEDED!! Still missing some.");
    for(i=0;i<THREADTEST_NRTHREADS;i++)
    {
        delete threads[i];
    }
}


void MultithreadTest::TestArabicShapingThreads()
{
    char threadTestChars[ARABICSHAPE_THREADTEST + 1];
    SimpleThread *threads[ARABICSHAPE_THREADTEST];
    int32_t numThreadsStarted = 0;

    int32_t i;

    for(i=0;i<ARABICSHAPE_THREADTEST;i++)
    {
        threadTestChars[i] = ' ';
        threads[i] = new TestArabicShapeThreads(&threadTestChars[i]);
    }
    threadTestChars[ARABICSHAPE_THREADTEST] = '\0';

    logln("-> do TestArabicShapingThreads <- Firing off threads.. ");
    for(i=0;i<ARABICSHAPE_THREADTEST;i++)
    {
        if (threads[i]->start() != 0) {
            errln("Error starting thread %d", i);
        }
        else {
            numThreadsStarted++;
        }
        
        logln(" Subthread started.");
    }

    logln("Waiting for threads to be set..");
    if (numThreadsStarted == 0) {
        errln("No threads could be started for testing!");
        return;
    }

    int32_t patience = 100; 

    while(patience--)
    {
        int32_t count = 0;
        umtx_lock(NULL);
        for(i=0;i<ARABICSHAPE_THREADTEST;i++)
        {
            if(threadTestChars[i] == '*')
            {
                count++;
            }
        }
        umtx_unlock(NULL);

        if(count == ARABICSHAPE_THREADTEST)
        {
            logln("->TestArabicShapingThreads <- Got all threads! cya");
            for(i=0;i<ARABICSHAPE_THREADTEST;i++)
            {
                delete threads[i];
            }
            return;
        }

        logln("-> TestArabicShapingThreads <- Waiting..");
        SimpleThread::sleep(500);
    }

    errln("-> TestArabicShapingThreads <- PATIENCE EXCEEDED!! Still missing some.");
    for(i=0;i<ARABICSHAPE_THREADTEST;i++)
    {
        delete threads[i];
    }
	
}










static UMutex    gTestMutexA = U_MUTEX_INITIALIZER;
static UMutex    gTestMutexB = U_MUTEX_INITIALIZER;

static int     gThreadsStarted = 0;
static int     gThreadsInMiddle = 0;
static int     gThreadsDone = 0;

static const int TESTMUTEX_THREAD_COUNT = 4;

static int safeIncr(int &var, int amt) {
    
    
    
    Mutex m;
    var += amt;
    return var;
}

class TestMutexThread : public SimpleThread
{
public:
    virtual void run()
    {
        
        
        
        
        safeIncr(gThreadsStarted, 1);
        umtx_lock(&gTestMutexA);
        umtx_unlock(&gTestMutexA);
        safeIncr(gThreadsInMiddle, 1);
        umtx_lock(&gTestMutexB);
        umtx_unlock(&gTestMutexB);
        safeIncr(gThreadsDone, 1);
    }
};

void MultithreadTest::TestMutex()
{
    
    
    
    gThreadsStarted = 0;
    gThreadsInMiddle = 0;
    gThreadsDone = 0;
    umtx_lock(&gTestMutexA);
    TestMutexThread  *threads[TESTMUTEX_THREAD_COUNT];
    int i;
    int32_t numThreadsStarted = 0;
    for (i=0; i<TESTMUTEX_THREAD_COUNT; i++) {
        threads[i] = new TestMutexThread;
        if (threads[i]->start() != 0) {
            errln("Error starting thread %d", i);
        }
        else {
            numThreadsStarted++;
        }
    }
    if (numThreadsStarted == 0) {
        errln("No threads could be started for testing!");
        return;
    }

    int patience = 0;
    while (safeIncr(gThreadsStarted, 0) != TESTMUTEX_THREAD_COUNT) {
        if (patience++ > 24) {
            TSMTHREAD_FAIL("Patience Exceeded");
            return;
        }
        SimpleThread::sleep(500);
    }
    
    TSMTHREAD_ASSERT(gThreadsInMiddle==0);
    TSMTHREAD_ASSERT(gThreadsDone==0);

    
    
    
    umtx_lock(&gTestMutexB);
    umtx_unlock(&gTestMutexA);

    patience = 0;
    while (safeIncr(gThreadsInMiddle, 0) != TESTMUTEX_THREAD_COUNT) {
        if (patience++ > 24) {
            TSMTHREAD_FAIL("Patience Exceeded");
            return;
        }
        SimpleThread::sleep(500);
    }
    TSMTHREAD_ASSERT(gThreadsDone==0);

    
    
    umtx_unlock(&gTestMutexB);
    patience = 0;
    while (safeIncr(gThreadsDone, 0) != TESTMUTEX_THREAD_COUNT) {
        if (patience++ > 24) {
            TSMTHREAD_FAIL("Patience Exceeded");
            return;
        }
        SimpleThread::sleep(500);
    }

    

    for (i=0; i<TESTMUTEX_THREAD_COUNT; i++) {
        delete threads[i];
    }

}







class ThreadWithStatus : public SimpleThread
{
public:
    UBool  getError() { return (fErrors > 0); }
    UBool  getError(UnicodeString& fillinError) { fillinError = fErrorString; return (fErrors > 0); }
    virtual ~ThreadWithStatus(){}
protected:
    ThreadWithStatus() :  fErrors(0) {}
    void error(const UnicodeString &error) {
        fErrors++; fErrorString = error;
        SimpleThread::errorFunc();
    }
    void error() { error("An error occured."); }
private:
    int32_t fErrors;
    UnicodeString fErrorString;
};











UnicodeString showDifference(const UnicodeString& expected, const UnicodeString& result)
{
    UnicodeString res;
    res = expected + "<Expected\n";
    if(expected.length() != result.length())
        res += " [ Different lengths ] \n";
    else
    {
        for(int32_t i=0;i<expected.length();i++)
        {
            if(expected[i] == result[i])
            {
                res += " ";
            }
            else
            {
                res += "|";
            }
        }
        res += "<Differences";
        res += "\n";
    }
    res += result + "<Result\n";

    return res;
}










const int kFormatThreadIterations = 100;  
const int kFormatThreadThreads    = 10;  

#if !UCONFIG_NO_FORMATTING



struct FormatThreadTestData
{
    double number;
    UnicodeString string;
    FormatThreadTestData(double a, const UnicodeString& b) : number(a),string(b) {}
} ;




static void formatErrorMessage(UErrorCode &realStatus, const UnicodeString& pattern, const Locale& theLocale,
                     UErrorCode inStatus0,  const Locale &inCountry2, double currency3, 
                     UnicodeString &result)
{
    if(U_FAILURE(realStatus))
        return; 

    UnicodeString errString1(u_errorName(inStatus0));

    UnicodeString countryName2;
    inCountry2.getDisplayCountry(theLocale,countryName2);

    Formattable myArgs[] = {
        Formattable((int32_t)inStatus0),   
        Formattable(errString1), 
        Formattable(countryName2),  
        Formattable(currency3)
    };

    MessageFormat *fmt = new MessageFormat("MessageFormat's API is broken!!!!!!!!!!!",realStatus);
    fmt->setLocale(theLocale);
    fmt->applyPattern(pattern, realStatus);

    if (U_FAILURE(realStatus)) {
        delete fmt;
        return;
    }

    FieldPosition ignore = 0;
    fmt->format(myArgs,4,result,ignore,realStatus);

    delete fmt;
}






class ThreadSafeFormatSharedData {
  public:
    ThreadSafeFormatSharedData(UErrorCode &status);
    ~ThreadSafeFormatSharedData();
    LocalPointer<NumberFormat>  fFormat;
    Formattable    fYDDThing;
    Formattable    fBBDThing;
    UnicodeString  fYDDStr;
    UnicodeString  fBBDStr;
};

const ThreadSafeFormatSharedData *gSharedData = NULL;

ThreadSafeFormatSharedData::ThreadSafeFormatSharedData(UErrorCode &status) {
    fFormat.adoptInstead(NumberFormat::createCurrencyInstance(Locale::getUS(), status));
    static const UChar kYDD[] = { 0x59, 0x44, 0x44, 0x00 };
    static const UChar kBBD[] = { 0x42, 0x42, 0x44, 0x00 };
    fYDDThing.adoptObject(new CurrencyAmount(123.456, kYDD, status));
    fBBDThing.adoptObject(new CurrencyAmount(987.654, kBBD, status));
    if (U_FAILURE(status)) {
        return;
    }
    fFormat->format(fYDDThing, fYDDStr, NULL, status);
    fFormat->format(fBBDThing, fBBDStr, NULL, status);
    gSharedData = this;
}

ThreadSafeFormatSharedData::~ThreadSafeFormatSharedData() {
    gSharedData = NULL;
}








class ThreadSafeFormat {
public:
  
  ThreadSafeFormat(UErrorCode &status);
  UBool doStuff(int32_t offset, UnicodeString &appendErr, UErrorCode &status) const;
private:
  LocalPointer<NumberFormat> fFormat; 
};


ThreadSafeFormat::ThreadSafeFormat(UErrorCode &status) {
  fFormat.adoptInstead(NumberFormat::createCurrencyInstance(Locale::getUS(), status));
}

static const UChar kUSD[] = { 0x55, 0x53, 0x44, 0x00 };

UBool ThreadSafeFormat::doStuff(int32_t offset, UnicodeString &appendErr, UErrorCode &status) const {
  UBool okay = TRUE;

  if(u_strcmp(fFormat->getCurrency(), kUSD)) {
    appendErr.append("fFormat currency != ")
      .append(kUSD)
      .append(", =")
      .append(fFormat->getCurrency())
      .append("! ");
    okay = FALSE;
  }

  if(u_strcmp(gSharedData->fFormat->getCurrency(), kUSD)) {
    appendErr.append("gFormat currency != ")
      .append(kUSD)
      .append(", =")
      .append(gSharedData->fFormat->getCurrency())
      .append("! ");
    okay = FALSE;
  }
  UnicodeString str;
  const UnicodeString *o=NULL;
  Formattable f;
  const NumberFormat *nf = NULL; 
  switch(offset%4) {
  case 0:  f = gSharedData->fYDDThing;  o = &gSharedData->fYDDStr;  nf = gSharedData->fFormat.getAlias();  break;
  case 1:  f = gSharedData->fBBDThing;  o = &gSharedData->fBBDStr;  nf = gSharedData->fFormat.getAlias();  break;
  case 2:  f = gSharedData->fYDDThing;  o = &gSharedData->fYDDStr;  nf = fFormat.getAlias();  break;
  case 3:  f = gSharedData->fBBDThing;  o = &gSharedData->fBBDStr;  nf = fFormat.getAlias();  break;
  }
  nf->format(f, str, NULL, status);

  if(*o != str) {
    appendErr.append(showDifference(*o, str));
    okay = FALSE;
  }
  return okay;
}

UBool U_CALLCONV isAcceptable(void *, const char *, const char *, const UDataInfo *) {
    return TRUE;
}





class FormatThreadTest : public ThreadWithStatus
{
public:
    int     fNum;
    int     fTraceInfo;

    LocalPointer<ThreadSafeFormat> fTSF;

    FormatThreadTest() 
        : ThreadWithStatus(),
        fNum(0),
        fTraceInfo(0),
        fTSF(NULL),
        fOffset(0)
        
    {
        UErrorCode status = U_ZERO_ERROR;      
        fTSF.adoptInstead(new ThreadSafeFormat(status));
        static int32_t fgOffset = 0;
        fgOffset += 3;
        fOffset = fgOffset;
    }


    virtual void run()
    {
        fTraceInfo                     = 1;
        LocalPointer<NumberFormat> percentFormatter;
        UErrorCode status = U_ZERO_ERROR;

#if 0
        
        for (int i=0; i<4000; i++) {
            status = U_ZERO_ERROR;
            UDataMemory *data1 = udata_openChoice(0, "res", "en_US", isAcceptable, 0, &status);
            UDataMemory *data2 = udata_openChoice(0, "res", "fr", isAcceptable, 0, &status);
            udata_close(data1);
            udata_close(data2);
            if (U_FAILURE(status)) {
                error("udata_openChoice failed.\n");
                break;
            }
        }
        return;
#endif

#if 0
        
        int m;
        for (m=0; m<4000; m++) {
            status         = U_ZERO_ERROR;
            UResourceBundle *res   = NULL;
            const char *localeName = NULL;

            Locale  loc = Locale::getEnglish();

            localeName = loc.getName();
            

            
            
            res = ures_open(NULL, localeName, &status);
            

            
            ures_close(res);
            

            if (U_FAILURE(status)) {
                error("Resource bundle construction failed.\n");
                break;
            }
        }
        return;
#endif

        
        FormatThreadTestData kNumberFormatTestData[] =
        {
            FormatThreadTestData((double)5.0, UnicodeString("5", "")),
                FormatThreadTestData( 6.0, UnicodeString("6", "")),
                FormatThreadTestData( 20.0, UnicodeString("20", "")),
                FormatThreadTestData( 8.0, UnicodeString("8", "")),
                FormatThreadTestData( 8.3, UnicodeString("8.3", "")),
                FormatThreadTestData( 12345, UnicodeString("12,345", "")),
                FormatThreadTestData( 81890.23, UnicodeString("81,890.23", "")),
        };
        int32_t kNumberFormatTestDataLength = UPRV_LENGTHOF(kNumberFormatTestData);

        
        FormatThreadTestData kPercentFormatTestData[] =
        {
            FormatThreadTestData((double)5.0, CharsToUnicodeString("500\\u00a0%")),
                FormatThreadTestData( 1.0, CharsToUnicodeString("100\\u00a0%")),
                FormatThreadTestData( 0.26, CharsToUnicodeString("26\\u00a0%")),
                FormatThreadTestData(
                   16384.99, CharsToUnicodeString("1\\u00a0638\\u00a0499\\u00a0%")), 
                FormatThreadTestData(
                    81890.23, CharsToUnicodeString("8\\u00a0189\\u00a0023\\u00a0%")),
        };
        int32_t kPercentFormatTestDataLength = UPRV_LENGTHOF(kPercentFormatTestData);
        int32_t iteration;

        status = U_ZERO_ERROR;
        LocalPointer<NumberFormat> formatter(NumberFormat::createInstance(Locale::getEnglish(),status));
        if(U_FAILURE(status)) {
            error("Error on NumberFormat::createInstance().");
            goto cleanupAndReturn;
        }

        percentFormatter.adoptInstead(NumberFormat::createPercentInstance(Locale::getFrench(),status));
        if(U_FAILURE(status))             {
            error("Error on NumberFormat::createPercentInstance().");
            goto cleanupAndReturn;
        }

        for(iteration = 0;!getError() && iteration<kFormatThreadIterations;iteration++)
        {

            int32_t whichLine = (iteration + fOffset)%kNumberFormatTestDataLength;

            UnicodeString  output;

            formatter->format(kNumberFormatTestData[whichLine].number, output);

            if(0 != output.compare(kNumberFormatTestData[whichLine].string)) {
                error("format().. expected " + kNumberFormatTestData[whichLine].string
                        + " got " + output);
                goto cleanupAndReturn;
            }

            
            output.remove();
            whichLine = (iteration + fOffset)%kPercentFormatTestDataLength;

            percentFormatter->format(kPercentFormatTestData[whichLine].number, output);
            if(0 != output.compare(kPercentFormatTestData[whichLine].string))
            {
                error("percent format().. \n" +
                        showDifference(kPercentFormatTestData[whichLine].string,output));
                goto cleanupAndReturn;
            }

            
            const int       kNumberOfMessageTests = 3;
            UErrorCode      statusToCheck;
            UnicodeString   patternToCheck;
            Locale          messageLocale;
            Locale          countryToCheck;
            double          currencyToCheck;

            UnicodeString   expected;

            
            switch((iteration+fOffset) % kNumberOfMessageTests)
            {
            default:
            case 0:
                statusToCheck=                      U_FILE_ACCESS_ERROR;
                patternToCheck=        "0:Someone from {2} is receiving a #{0}"
                                       " error - {1}. Their telephone call is costing "
                                       "{3,number,currency}."; 
                messageLocale=                      Locale("en","US");
                countryToCheck=                     Locale("","HR");
                currencyToCheck=                    8192.77;
                expected=  "0:Someone from Croatia is receiving a #4 error - "
                            "U_FILE_ACCESS_ERROR. Their telephone call is costing $8,192.77.";
                break;
            case 1:
                statusToCheck=                      U_INDEX_OUTOFBOUNDS_ERROR;
                patternToCheck=                     "1:A customer in {2} is receiving a #{0} error - {1}. "
                                                    "Their telephone call is costing {3,number,currency}."; 
                messageLocale=                      Locale("de","DE@currency=DEM");
                countryToCheck=                     Locale("","BF");
                currencyToCheck=                    2.32;
                expected=                           CharsToUnicodeString(
                                                    "1:A customer in Burkina Faso is receiving a #8 error - U_INDEX_OUTOFBOUNDS_ERROR. "
                                                    "Their telephone call is costing 2,32\\u00A0DM.");
                break;
            case 2:
                statusToCheck=                      U_MEMORY_ALLOCATION_ERROR;
                patternToCheck=   "2:user in {2} is receiving a #{0} error - {1}. "
                                  "They insist they just spent {3,number,currency} "
                                  "on memory."; 
                messageLocale=                      Locale("de","AT@currency=ATS"); 
                countryToCheck=                     Locale("","US"); 
                currencyToCheck=                    40193.12;
                expected=       CharsToUnicodeString(
                            "2:user in Vereinigte Staaten is receiving a #7 error"
                            " - U_MEMORY_ALLOCATION_ERROR. They insist they just spent"
                            " \\u00f6S\\u00A040.193,12 on memory.");
                break;
            }

            UnicodeString result;
            UErrorCode status = U_ZERO_ERROR;
            formatErrorMessage(status,patternToCheck,messageLocale,statusToCheck,
                                countryToCheck,currencyToCheck,result);
            if(U_FAILURE(status))
            {
                UnicodeString tmp(u_errorName(status));
                error("Failure on message format, pattern=" + patternToCheck +
                        ", error = " + tmp);
                goto cleanupAndReturn;
            }

            if(result != expected)
            {
                error("PatternFormat: \n" + showDifference(expected,result));
                goto cleanupAndReturn;
            }
            
            UnicodeString appendErr;
            if(!fTSF->doStuff(fNum, appendErr, status)) {
              error(appendErr);
              goto cleanupAndReturn;
            }
        }   



cleanupAndReturn:
        
        fTraceInfo = 2;
    }

private:
    int32_t fOffset; 
};



void MultithreadTest::TestThreadedIntl()
{
    int i;
    UnicodeString theErr;
    UBool   haveDisplayedInfo[kFormatThreadThreads];
    static const int32_t PATIENCE_SECONDS = 45;

    UErrorCode threadSafeErr = U_ZERO_ERROR;

    ThreadSafeFormatSharedData sharedData(threadSafeErr);
    assertSuccess("initializing ThreadSafeFormat", threadSafeErr, TRUE);

    
    
    
    logln("Spawning: %d threads * %d iterations each.",
                kFormatThreadThreads, kFormatThreadIterations);
    LocalArray<FormatThreadTest> tests(new FormatThreadTest[kFormatThreadThreads]);
    for(int32_t j = 0; j < kFormatThreadThreads; j++) {
        tests[j].fNum = j;
        int32_t threadStatus = tests[j].start();
        if (threadStatus != 0) {
            errln("System Error %d starting thread number %d.", threadStatus, j);
            SimpleThread::errorFunc();
            return;
        }
        haveDisplayedInfo[j] = FALSE;
    }


    
    UBool   stillRunning;
    UDate startTime, endTime;
    startTime = Calendar::getNow();
    double lastComplaint = 0;
    do {
        
        stillRunning = FALSE;
        endTime = Calendar::getNow();
        double elapsedSeconds =  ((int32_t)(endTime - startTime)/U_MILLIS_PER_SECOND);
        if (elapsedSeconds > PATIENCE_SECONDS) {
            errln("Patience exceeded. Test is taking too long.");
            return;
        } else if((elapsedSeconds-lastComplaint) > 2.0) {
            infoln("%.1f seconds elapsed (still waiting..)", elapsedSeconds);
            lastComplaint = elapsedSeconds;
        }
        




        SimpleThread::sleep(1); 
        for(i=0;i<kFormatThreadThreads;i++) {
            if (tests[i].isRunning()) {
                stillRunning = TRUE;
            } else if (haveDisplayedInfo[i] == FALSE) {
                logln("Thread # %d is complete..", i);
                if(tests[i].getError(theErr)) {
                    dataerrln(UnicodeString("#") + i + ": " + theErr);
                    SimpleThread::errorFunc();
                }
                haveDisplayedInfo[i] = TRUE;
            }
        }
    } while (stillRunning);

    
    
    
    assertSuccess("finalizing ThreadSafeFormat", threadSafeErr, TRUE);
}
#endif 










#if !UCONFIG_NO_COLLATION

#define kCollatorThreadThreads   10  // # of threads to spawn
#define kCollatorThreadPatience kCollatorThreadThreads*30

struct Line {
    UChar buff[25];
    int32_t buflen;
} ;

static UBool
skipLineBecauseOfBug(const UChar *s, int32_t length) {
    
    if(length >= 3 &&
            (s[0] == 0xfb2 || s[0] == 0xfb3) &&
            s[1] == 0x334 &&
            (s[2] == 0xf73 || s[2] == 0xf75 || s[2] == 0xf81)) {
        return TRUE;
    }
    return FALSE;
}

static UCollationResult
normalizeResult(int32_t result) {
    return result<0 ? UCOL_LESS : result==0 ? UCOL_EQUAL : UCOL_GREATER;
}

class CollatorThreadTest : public ThreadWithStatus
{
private:
    const Collator *coll;
    const Line *lines;
    int32_t noLines;
    UBool isAtLeastUCA62;
public:
    CollatorThreadTest()  : ThreadWithStatus(),
        coll(NULL),
        lines(NULL),
        noLines(0),
        isAtLeastUCA62(TRUE)
    {
    };
    void setCollator(Collator *c, Line *l, int32_t nl, UBool atLeastUCA62)
    {
        coll = c;
        lines = l;
        noLines = nl;
        isAtLeastUCA62 = atLeastUCA62;
    }
    virtual void run() {
        uint8_t sk1[1024], sk2[1024];
        uint8_t *oldSk = NULL, *newSk = sk1;
        int32_t oldLen = 0;
        int32_t prev = 0;
        int32_t i = 0;

        for(i = 0; i < noLines; i++) {
            if(lines[i].buflen == 0) { continue; }

            if(skipLineBecauseOfBug(lines[i].buff, lines[i].buflen)) { continue; }

            int32_t resLen = coll->getSortKey(lines[i].buff, lines[i].buflen, newSk, 1024);

            if(oldSk != NULL) {
                int32_t skres = strcmp((char *)oldSk, (char *)newSk);
                int32_t cmpres = coll->compare(lines[prev].buff, lines[prev].buflen, lines[i].buff, lines[i].buflen);
                int32_t cmpres2 = coll->compare(lines[i].buff, lines[i].buflen, lines[prev].buff, lines[prev].buflen);

                if(cmpres != -cmpres2) {
                    error(UnicodeString("Compare result not symmetrical on line ") + (i + 1));
                    break;
                }

                if(cmpres != normalizeResult(skres)) {
                    error(UnicodeString("Difference between coll->compare and sortkey compare on line ") + (i + 1));
                    break;
                }

                int32_t res = cmpres;
                if(res == 0 && !isAtLeastUCA62) {
                    
                    
                    res = u_strcmpCodePointOrder(lines[prev].buff, lines[i].buff);
                    
                    
                    
                }
                if(res > 0) {
                    error(UnicodeString("Line is not greater or equal than previous line, for line ") + (i + 1));
                    break;
                }
            }

            oldSk = newSk;
            oldLen = resLen;
            (void)oldLen;   
            prev = i;

            newSk = (newSk == sk1)?sk2:sk1;
        }
    }
};

void MultithreadTest::TestCollators()
{

    UErrorCode status = U_ZERO_ERROR;
    FILE *testFile = NULL;
    char testDataPath[1024];
    strcpy(testDataPath, IntlTest::getSourceTestData(status));
    if (U_FAILURE(status)) {
        errln("ERROR: could not open test data %s", u_errorName(status));
        return;
    }
    strcat(testDataPath, "CollationTest_");

    const char* type = "NON_IGNORABLE";

    const char *ext = ".txt";
    if(testFile) {
        fclose(testFile);
    }
    char buffer[1024];
    strcpy(buffer, testDataPath);
    strcat(buffer, type);
    size_t bufLen = strlen(buffer);

    
    
    
    
    

    strcpy(buffer+bufLen, ext);

    testFile = fopen(buffer, "rb");

    if(testFile == 0) {
        strcpy(buffer+bufLen, "_SHORT");
        strcat(buffer, ext);
        testFile = fopen(buffer, "rb");

        if(testFile == 0) {
            strcpy(buffer+bufLen, "_STUB");
            strcat(buffer, ext);
            testFile = fopen(buffer, "rb");

            if (testFile == 0) {
                *(buffer+bufLen) = 0;
                dataerrln("could not open any of the conformance test files, tried opening base %s", buffer);
                return;
            } else {
                infoln(
                    "INFO: Working with the stub file.\n"
                    "If you need the full conformance test, please\n"
                    "download the appropriate data files from:\n"
                    "http://source.icu-project.org/repos/icu/tools/trunk/unicodetools/com/ibm/text/data/");
            }
        }
    }

    LocalArray<Line> lines(new Line[200000]);
    memset(lines.getAlias(), 0, sizeof(Line)*200000);
    int32_t lineNum = 0;

    UChar bufferU[1024];
    uint32_t first = 0;

    while (fgets(buffer, 1024, testFile) != NULL) {
        if(*buffer == 0 || buffer[0] == '#') {
            
            
            lines[lineNum].buflen = 0;
            lines[lineNum].buff[0] = 0;
        } else {
            int32_t buflen = u_parseString(buffer, bufferU, 1024, &first, &status);
            lines[lineNum].buflen = buflen;
            u_memcpy(lines[lineNum].buff, bufferU, buflen);
            lines[lineNum].buff[buflen] = 0;
        }
        lineNum++;
    }
    fclose(testFile);
    if(U_FAILURE(status)) {
      dataerrln("Couldn't read the test file!");
      return;
    }

    UVersionInfo uniVersion;
    static const UVersionInfo v62 = { 6, 2, 0, 0 };
    u_getUnicodeVersion(uniVersion);
    UBool isAtLeastUCA62 = uprv_memcmp(uniVersion, v62, 4) >= 0;

    LocalPointer<Collator> coll(Collator::createInstance(Locale::getRoot(), status));
    if(U_FAILURE(status)) {
        errcheckln(status, "Couldn't open UCA collator");
        return;
    }
    coll->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    coll->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    coll->setAttribute(UCOL_CASE_LEVEL, UCOL_OFF, status);
    coll->setAttribute(UCOL_STRENGTH, isAtLeastUCA62 ? UCOL_IDENTICAL : UCOL_TERTIARY, status);
    coll->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, status);

    int32_t noSpawned = 0;
    int32_t spawnResult = 0;
    LocalArray<CollatorThreadTest> tests(new CollatorThreadTest[kCollatorThreadThreads]);

    logln(UnicodeString("Spawning: ") + kCollatorThreadThreads + " threads * " + kFormatThreadIterations + " iterations each.");
    int32_t j = 0;
    for(j = 0; j < kCollatorThreadThreads; j++) {
        
        tests[j].setCollator(coll.getAlias(), lines.getAlias(), lineNum, isAtLeastUCA62);
    }
    for(j = 0; j < kCollatorThreadThreads; j++) {
        log("%i ", j);
        spawnResult = tests[j].start();
        if(spawnResult != 0) {
            infoln("THREAD INFO: Couldn't spawn more than %i threads", noSpawned);
            break;
        }
        noSpawned++;
    }
    logln("Spawned all");
    if (noSpawned == 0) {
        errln("No threads could be spawned.");
        return;
    }

    for(int32_t patience = kCollatorThreadPatience;patience > 0; patience --)
    {
        logln("Waiting...");

        int32_t i;
        int32_t terrs = 0;
        int32_t completed =0;

        for(i=0;i<kCollatorThreadThreads;i++)
        {
            if (tests[i].isRunning() == FALSE)
            {
                completed++;

                

                UnicodeString theErr;
                if(tests[i].getError(theErr))
                {
                    terrs++;
                    errln(UnicodeString("#") + i + ": " + theErr);
                }
                
            }
        }
        logln("Completed %i tests", completed);

        if(completed == noSpawned)
        {
            logln("Done! All %i tests are finished", noSpawned);

            if(terrs)
            {
                errln("There were errors.");
                SimpleThread::errorFunc();
            }
            return;
        }

        SimpleThread::sleep(900);
    }
    errln("patience exceeded. ");
    SimpleThread::errorFunc();
}

#endif 










const int kStringThreadIterations = 2500;
const int kStringThreadThreads    = 10;  
const int kStringThreadPatience   = 120; 


class StringThreadTest2 : public ThreadWithStatus
{
public:
    int                 fNum;
    int                 fTraceInfo;
    const UnicodeString *fSharedString;

    StringThreadTest2(const UnicodeString *sharedString, int num) 
        : ThreadWithStatus(),
        fNum(num),
        fTraceInfo(0),
        fSharedString(sharedString)
    {
    };


    virtual void run()
    {
        fTraceInfo    = 1;
        int loopCount = 0;

        for (loopCount = 0; loopCount < kStringThreadIterations; loopCount++) {
            if (*fSharedString != "This is the original test string.") {
                error("Original string is corrupt.");
                break;
            }
            UnicodeString s1 = *fSharedString;
            s1 += "cat this";
            UnicodeString s2(s1);
            UnicodeString s3 = *fSharedString;
            s2 = s3;
            s3.truncate(12);
            s2.truncate(0);
        }

        
        fTraceInfo = 2;
    }

};



void MultithreadTest::TestString()
{
    int     patience;
    int     terrs = 0;
    int     j;

    UnicodeString *testString = new UnicodeString("This is the original test string.");

    
    
    
    StringThreadTest2  *tests[kStringThreadThreads];
    for(j = 0; j < kStringThreadThreads; j++) {
        tests[j] = new StringThreadTest2(testString, j);
    }

    logln(UnicodeString("Spawning: ") + kStringThreadThreads + " threads * " + kStringThreadIterations + " iterations each.");
    for(j = 0; j < kStringThreadThreads; j++) {
        int32_t threadStatus = tests[j]->start();
        if (threadStatus != 0) {
            errln("System Error %d starting thread number %d.", threadStatus, j);
            SimpleThread::errorFunc();
            goto cleanupAndReturn;
        }
    }

    for(patience = kStringThreadPatience;patience > 0; patience --)
    {
        logln("Waiting...");

        int32_t i;
        terrs = 0;
        int32_t completed =0;

        for(i=0;i<kStringThreadThreads;i++) {
            if (tests[i]->isRunning() == FALSE)
            {
                completed++;

                logln(UnicodeString("Test #") + i + " is complete.. ");

                UnicodeString theErr;
                if(tests[i]->getError(theErr))
                {
                    terrs++;
                    errln(UnicodeString("#") + i + ": " + theErr);
                }
                
            }
        }

        if(completed == kStringThreadThreads)
        {
            logln("Done!");
            if(terrs) {
                errln("There were errors.");
            }
            break;
        }

        SimpleThread::sleep(900);
    }

    if (patience <= 0) {
        errln("patience exceeded. ");
        
        terrs++;
    }

    if (terrs > 0) {
        SimpleThread::errorFunc();
    }

cleanupAndReturn:
    if (terrs == 0) {
        





        for(j = 0; j < kStringThreadThreads; j++) {
            delete tests[j];
        }
        delete testString;
    }
}







#if !UCONFIG_NO_TRANSLITERATION
class TxThread: public SimpleThread {
  private:
    Transliterator *fSharedTranslit;
  public:
    UBool fSuccess;
    TxThread(Transliterator *tx) : fSharedTranslit(tx), fSuccess(FALSE) {};
    ~TxThread();
    void run();
};

TxThread::~TxThread() {}
void TxThread::run() {
    UnicodeString greekString("\\u03B4\\u03B9\\u03B1\\u03C6\\u03BF\\u03C1\\u03B5\\u03C4\\u03B9\\u03BA\\u03BF\\u03CD\\u03C2");
    greekString = greekString.unescape();
    fSharedTranslit->transliterate(greekString);
    fSuccess = greekString[0] == 0x64; 
}
#endif


void MultithreadTest::TestAnyTranslit() {
#if !UCONFIG_NO_TRANSLITERATION
    UErrorCode status = U_ZERO_ERROR;
    LocalPointer<Transliterator> tx(Transliterator::createInstance("Any-Latin", UTRANS_FORWARD, status));
    if (U_FAILURE(status)) {
        dataerrln("File %s, Line %d: Error, status = %s", __FILE__, __LINE__, u_errorName(status));
        return;
    }
    TxThread * threads[4];
    int32_t i;
    for (i=0; i<4; i++) {
        threads[i] = new TxThread(tx.getAlias());
    }
    for (i=0; i<4; i++) {
        threads[i]->start();
    }
    int32_t patience = 100;
    UBool success;
    UBool someThreadRunning;
    do {
        someThreadRunning = FALSE;
        success = TRUE;
        for (i=0; i<4; i++) {
            if (threads[i]->isRunning()) {
                someThreadRunning = TRUE;
                SimpleThread::sleep(10);
                break;
            } else {
                if (threads[i]->fSuccess == FALSE) {
                    success = FALSE;
                }
            }
        }
    } while (someThreadRunning && --patience > 0);

    if (patience <= 0) {
        errln("File %s, Line %d: Error, one or more threads did not complete.", __FILE__, __LINE__);
    }
    if (success == FALSE) {
        errln("File %s, Line %d: Error, transliteration result incorrect.", __FILE__, __LINE__);
    }

    for (i=0; i<4; i++) {
        delete threads[i];
    }
#endif  
}












class CondThread: public SimpleThread {
  public:
    CondThread() :fFinished(false)  {};
    ~CondThread() {};
    void run();
    bool  fFinished;
};

static UMutex gCTMutex = U_MUTEX_INITIALIZER;
static UConditionVar gCTConditionVar = U_CONDITION_INITIALIZER;
int gConditionTestOne = 1;   
                             
int gStartedThreads;
int gFinishedThreads;
static const int NUMTHREADS = 10;

static MultithreadTest *gThisTest = NULL; 
                                          


void CondThread::run() {
    umtx_lock(&gCTMutex);
    gStartedThreads += gConditionTestOne;
    umtx_condBroadcast(&gCTConditionVar);

    while (gStartedThreads < NUMTHREADS) {
        if (gFinishedThreads != 0) {
            gThisTest->errln("File %s, Line %d: Error, gStartedThreads = %d, gFinishedThreads = %d",
                             __FILE__, __LINE__, gStartedThreads, gFinishedThreads);
        }
        umtx_condWait(&gCTConditionVar, &gCTMutex);
    }

    gFinishedThreads += gConditionTestOne;
    fFinished = true;
    umtx_condBroadcast(&gCTConditionVar);

    while (gFinishedThreads < NUMTHREADS) {
        umtx_condWait(&gCTConditionVar, &gCTMutex);
    }
    umtx_unlock(&gCTMutex);
}

void MultithreadTest::TestConditionVariables() {
    gThisTest = this;
    gStartedThreads = 0;
    gFinishedThreads = 0;
    int i;

    umtx_lock(&gCTMutex);
    CondThread *threads[NUMTHREADS];
    for (i=0; i<NUMTHREADS; ++i) {
        threads[i] = new CondThread;
        threads[i]->start();
    }

    while (gStartedThreads < NUMTHREADS) {
        umtx_condWait(&gCTConditionVar, &gCTMutex);
    }

    while (gFinishedThreads < NUMTHREADS) {
        umtx_condWait(&gCTConditionVar, &gCTMutex);
    }

    umtx_unlock(&gCTMutex);

    for (i=0; i<NUMTHREADS; ++i) {
        if (!threads[i]->fFinished) {
            errln("File %s, Line %d: Error, threads[%d]->fFinished == false", __FILE__, __LINE__, i);
        }
        delete threads[i];
    }
}

static const char *gCacheLocales[] = {"en_US", "en_GB", "fr_FR", "fr"};
static int32_t gObjectsCreated = 0;
static const int32_t CACHE_LOAD = 3;

class UCTMultiThreadItem : public SharedObject {
  public:
    char *value;
    UCTMultiThreadItem(const char *x) : value(NULL) {
        value = uprv_strdup(x);
    }
    virtual ~UCTMultiThreadItem() {
        uprv_free(value);
    }
};

U_NAMESPACE_BEGIN

template<> U_EXPORT
const UCTMultiThreadItem *LocaleCacheKey<UCTMultiThreadItem>::createObject(
        const void * , UErrorCode & ) const {
    
    
    umtx_lock(&gCTMutex);
    if (gObjectsCreated != 0) {
        gThisTest->errln("Expected no objects to be created yet.");
    }
    umtx_unlock(&gCTMutex);

    
    SimpleThread::sleep(1000);

    
    umtx_lock(&gCTMutex);
    ++gObjectsCreated;
    umtx_unlock(&gCTMutex);
    UCTMultiThreadItem *result = new UCTMultiThreadItem(fLoc.getName());
    result->addRef();
    return result;
}

U_NAMESPACE_END

class UnifiedCacheThread: public SimpleThread {
  public:
    UnifiedCacheThread(const char *loc) : fLoc(loc) {};
    ~UnifiedCacheThread() {};
    void run();
    const char *fLoc;
};

void UnifiedCacheThread::run() {
    UErrorCode status = U_ZERO_ERROR;
    const UnifiedCache *cache = UnifiedCache::getInstance(status);
    U_ASSERT(status == U_ZERO_ERROR);
    const UCTMultiThreadItem *item = NULL;
    cache->get(LocaleCacheKey<UCTMultiThreadItem>(fLoc), item, status);
    U_ASSERT(item != NULL);
    if (uprv_strcmp(fLoc, item->value)) {
      gThisTest->errln("Expected %s, got %s", fLoc, item->value);
    }
    item->removeRef();

    
    umtx_lock(&gCTMutex);
    ++gFinishedThreads;
    umtx_condBroadcast(&gCTConditionVar);
    umtx_unlock(&gCTMutex);
}

void MultithreadTest::TestUnifiedCache() {
    UErrorCode status = U_ZERO_ERROR;
    const UnifiedCache *cache = UnifiedCache::getInstance(status);
    U_ASSERT(cache != NULL);
    cache->flush();
    gThisTest = this;
    gFinishedThreads = 0;
    gObjectsCreated = 0;

    UnifiedCacheThread *threads[CACHE_LOAD][UPRV_LENGTHOF(gCacheLocales)];
    for (int32_t i=0; i<CACHE_LOAD; ++i) {
        for (int32_t j=0; j<UPRV_LENGTHOF(gCacheLocales); ++j) {
            threads[i][j] = new UnifiedCacheThread(gCacheLocales[j]);
            threads[i][j]->start();
        }
    }
    
    
    umtx_lock(&gCTMutex);
    while (gFinishedThreads < CACHE_LOAD*UPRV_LENGTHOF(gCacheLocales)) {
        umtx_condWait(&gCTConditionVar, &gCTMutex);
    }
    assertEquals("Objects created", UPRV_LENGTHOF(gCacheLocales), gObjectsCreated);
    umtx_unlock(&gCTMutex);

    
    for (int32_t i=0; i<CACHE_LOAD; ++i) {
        for (int32_t j=0; j<UPRV_LENGTHOF(gCacheLocales); ++j) {
            delete threads[i][j];
        }
    }
}

#endif 
