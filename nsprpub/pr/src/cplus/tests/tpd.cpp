









































#include "prlog.h"
#include "prprf.h"
#include "rcthread.h"

#include <string.h>

#include "plgetopt.h"




class MyThread: public RCThread
{
public:
    MyThread();

private:
    ~MyThread();
    void RootFunction();
};  




class MyPrivateData: public RCThreadPrivateData
{
public:
    virtual ~MyPrivateData();

    MyPrivateData();
    MyPrivateData(char*);
    MyPrivateData(const MyPrivateData&);

    void Release();

private:
    char *string;
};  

static PRUintn key[128];
static PRIntn debug = 0;
static PRBool failed = PR_FALSE;
static PRBool should = PR_TRUE;
static PRBool did = PR_TRUE;
static PRFileDesc *fout = NULL;

static void PrintProgress(PRIntn line)
{
    failed = failed || (should && !did);
    failed = failed || (!should && did);
    if (debug > 0)
    {
        PR_fprintf(
            fout, "@ line %d destructor should %shave been called and was%s\n",
            line, ((should) ? "" : "NOT "), ((did) ? "" : " NOT"));
    }
}  

static void MyAssert(const char *expr, const char *file, PRIntn line)
{
    if (debug > 0)
        (void)PR_fprintf(fout, "'%s' in file: %s: %d\n", expr, file, line);
}  

#define MY_ASSERT(_expr) \
    ((_expr)?((void)0):MyAssert(# _expr,__FILE__,__LINE__))

int main(PRIntn argc, char *argv[])
{
    PRStatus rv;
    PRUintn keys;
    MyThread *thread;
    const RCThreadPrivateData *pd;
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "d");
    RCThread *primordial = RCThread::WrapPrimordialThread();
    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
            debug = PR_TRUE;
            break;
         default:
            break;
        }
    }
    PL_DestroyOptState(opt);

    fout = PR_STDOUT;

    MyPrivateData extension = MyPrivateData("EXTENSION");
    MyPrivateData key_string[] = {
        "Key #0", "Key #1", "Key #2", "Key #3",
        "Bogus #5", "Bogus #6", "Bogus #7", "Bogus #8"};
    

    did = should = PR_FALSE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = RCThread::NewPrivateIndex(&key[keys]);
        key[keys + 4] = key[keys] + 4;
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    
    did = should = PR_FALSE;
    for (keys = 0; keys < 8; ++keys)
    {
        pd = RCThread::GetPrivateData(key[keys]);
        MY_ASSERT(NULL == pd);
    }
    PrintProgress(__LINE__);

    
    did = should = PR_FALSE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = RCThread::SetPrivateData(key[keys], &key_string[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

        
    did = PR_FALSE; should = PR_TRUE;
    for (keys = 0; keys < 4; ++keys)
    {
        pd = RCThread::GetPrivateData(key[keys]);
        PR_ASSERT(NULL != pd);
        rv = RCThread::SetPrivateData(key[keys], &key_string[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    
    did = PR_FALSE; should = PR_TRUE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = RCThread::SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    
    did = should = PR_FALSE;
    for (keys = 0; keys < 4; ++keys)
    {
        pd = RCThread::GetPrivateData(key[keys]);
        PR_ASSERT(NULL == pd);
    }
    PrintProgress(__LINE__);

    
    did = should = PR_FALSE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = RCThread::NewPrivateIndex(&key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
        rv = RCThread::SetPrivateData(key[keys], &extension);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    
    did = PR_FALSE; should = PR_TRUE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = RCThread::SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    
    did = should = PR_FALSE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = RCThread::SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }

    if (debug) PR_fprintf(fout, "Creating thread\n");
    thread = new MyThread();
    if (debug) PR_fprintf(fout, "Starting thread\n");
    thread->Start();
    if (debug) PR_fprintf(fout, "Joining thread\n");
    (void)thread->Join();
    if (debug) PR_fprintf(fout, "Joined thread\n");

    failed |= (PR_FAILURE == RCPrimordialThread::Cleanup());

    (void)PR_fprintf(
        fout, "%s\n",((PR_TRUE == failed) ? "FAILED" : "PASSED"));

    return (failed) ? 1 : 0;

}  




MyPrivateData::~MyPrivateData()
{
    PR_fprintf(
        fout, "MyPrivateData::~MyPrivateData[%s]\n",
        (NULL != string) ? string : "NULL");
}  

MyPrivateData::MyPrivateData(): RCThreadPrivateData()
{
    PR_fprintf(fout, "MyPrivateData::MyPrivateData()\n");
    string = NULL;
}  

MyPrivateData::MyPrivateData(char* data): RCThreadPrivateData()
{
    PR_fprintf(fout, "MyPrivateData::MyPrivateData(char* data)\n");
    string = data;
}  

MyPrivateData::MyPrivateData(const MyPrivateData& him): RCThreadPrivateData(him)
{
    PR_fprintf(fout, "MyPrivateData::MyPrivateData(const MyPrivateData& him)\n");
    string = him.string;
}  

void MyPrivateData::Release()
{
    if (should) did = PR_TRUE;
    else failed = PR_TRUE;
}  




MyThread::~MyThread() { }
MyThread::MyThread(): RCThread(RCThread::global, RCThread::joinable) { }


void MyThread::RootFunction()
{
    PRStatus rv;
    PRUintn keys;
    const RCThreadPrivateData *pd;
    
    MyPrivateData extension = MyPrivateData("EXTENSION");
    MyPrivateData key_string[] = {
        "Key #0", "Key #1", "Key #2", "Key #3",
        "Bogus #5", "Bogus #6", "Bogus #7", "Bogus #8"};
    
    did = should = PR_FALSE;
    for (keys = 0; keys < 8; ++keys)
    {
        pd = GetPrivateData(key[keys]);
        MY_ASSERT(NULL == pd);
    }
    PrintProgress(__LINE__);

    did = should = PR_FALSE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = SetPrivateData(keys, &key_string[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

#if !defined(DEBUG)
    did = should = PR_FALSE;
    for (keys = 4; keys < 8; ++keys)
    {
        rv = SetPrivateData(keys, &key_string[keys]);
        MY_ASSERT(PR_FAILURE == rv);
    }
    PrintProgress(__LINE__);
#endif
    
    did = PR_FALSE; should = PR_TRUE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = SetPrivateData(key[keys], &key_string[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    did = PR_FALSE; should = PR_TRUE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    did = should = PR_FALSE;
    for (keys = 0; keys < 4; ++keys)
    {
        rv = SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    did = should = PR_FALSE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = SetPrivateData(key[keys], &extension);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    did = PR_FALSE; should = PR_TRUE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
    PrintProgress(__LINE__);

    did = should = PR_FALSE;
    for (keys = 8; keys < 127; ++keys)
    {
        rv = SetPrivateData(key[keys]);
        MY_ASSERT(PR_SUCCESS == rv);
    }
}  


