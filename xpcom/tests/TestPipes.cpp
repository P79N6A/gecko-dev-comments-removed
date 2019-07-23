





































#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIServiceManager.h"
#include "nsThreadUtils.h"
#include "prprf.h"
#include "prinrval.h"
#include "plstr.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include <stdio.h>
#include "nsIPipe.h"    
#include "nsAutoLock.h"
#include <stdlib.h>     

#define KEY             0xa7
#define ITERATIONS      33333
char kTestPattern[] = "My hovercraft is full of eels.\n";

PRBool gTrace = PR_FALSE;

static nsresult
WriteAll(nsIOutputStream *os, const char *buf, PRUint32 bufLen, PRUint32 *lenWritten)
{
    const char *p = buf;
    *lenWritten = 0;
    while (bufLen) {
        PRUint32 n;
        nsresult rv = os->Write(p, bufLen, &n);
        if (NS_FAILED(rv)) return rv;
        p += n;
        bufLen -= n;
        *lenWritten += n;
    }
    return NS_OK;
}

class nsReceiver : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        char buf[101];
        PRUint32 count;
        PRIntervalTime start = PR_IntervalNow();
        while (PR_TRUE) {
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {

                break;
            }

            if (gTrace) {
                buf[count] = '\0';
                printf("read: %s\n", buf);
            }
            mCount += count;
        }
        PRIntervalTime end = PR_IntervalNow();
        printf("read  %d bytes, time = %dms\n", mCount,
               PR_IntervalToMilliseconds(end - start));
        return rv;
    }

    nsReceiver(nsIInputStream* in) : mIn(in), mCount(0) {
        NS_ADDREF(in);
    }

    PRUint32 GetBytesRead() { return mCount; }

private:
    ~nsReceiver() {
        NS_RELEASE(mIn);
    }

protected:
    nsIInputStream*     mIn;
    PRUint32            mCount;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsReceiver, nsIRunnable)

nsresult
TestPipe(nsIInputStream* in, nsIOutputStream* out)
{
    nsresult rv;
    nsIThread* thread;
    nsReceiver* receiver = new nsReceiver(in);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&thread, receiver);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    PRIntervalTime start = PR_IntervalNow();
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char *buf = PR_smprintf("%d %s", i, kTestPattern);
        PRUint32 len = strlen(buf);
        rv = WriteAll(out, buf, len, &writeCount);
        if (gTrace) {
            printf("wrote: ");
            for (PRUint32 j = 0; j < writeCount; j++) {
                putc(buf[j], stdout);
            }
            printf("\n");
        }
        PR_smprintf_free(buf);
        if (NS_FAILED(rv)) return rv;
        total += writeCount;
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    PRIntervalTime end = PR_IntervalNow();

    thread->Shutdown();

    printf("wrote %d bytes, time = %dms\n", total,
           PR_IntervalToMilliseconds(end - start));
    NS_ASSERTION(receiver->GetBytesRead() == total, "didn't read everything");

    NS_RELEASE(thread);
    NS_RELEASE(receiver);

    return NS_OK;
}



class nsShortReader : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        char buf[101];
        PRUint32 count;
        PRUint32 total = 0;
        while (PR_TRUE) {
            
            
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {
                break;
            }

            if (gTrace) {
                
                buf[count] = '\0';

                printf("read %d bytes: %s\n", count, buf);
            }

            Received(count);
            total += count;
        }
        printf("read  %d bytes\n", total);
        return rv;
    }

    nsShortReader(nsIInputStream* in) : mIn(in), mReceived(0) {
        NS_ADDREF(in);
    }

    void Received(PRUint32 count) {
        nsAutoCMonitor mon(this);
        mReceived += count;
        mon.Notify();
    }

    PRUint32 WaitForReceipt(const PRUint32 aWriteCount) {
        nsAutoCMonitor mon(this);
        PRUint32 result = mReceived;

        while (result < aWriteCount) {
            mon.Wait();

            NS_ASSERTION(mReceived > result, "failed to receive");
            result = mReceived;
        }

        mReceived = 0;
        return result;
    }

private:
    ~nsShortReader() {
        NS_RELEASE(mIn);
    }

protected:
    nsIInputStream*     mIn;
    PRUint32            mReceived;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsShortReader, nsIRunnable)

nsresult
TestShortWrites(nsIInputStream* in, nsIOutputStream* out)
{
    nsresult rv;
    nsIThread* thread;
    nsShortReader* receiver = new nsShortReader(in);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&thread, receiver);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        PRUint32 len = strlen(buf);
        len = len * rand() / RAND_MAX;
        len = PR_MAX(1, len);
        rv = WriteAll(out, buf, len, &writeCount);
        if (NS_FAILED(rv)) return rv;
        NS_ASSERTION(writeCount == len, "didn't write enough");
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);
        PR_smprintf_free(buf);
        
        out->Flush();
        
        const PRUint32 received = receiver->WaitForReceipt(writeCount);
        NS_ASSERTION(received == writeCount, "received wrong amount");
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    thread->Shutdown();
    printf("wrote %d bytes\n", total);

    NS_RELEASE(thread);
    NS_RELEASE(receiver);

    return NS_OK;
}



class nsPump : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        PRUint32 count;
        while (PR_TRUE) {
            rv = mOut->WriteFrom(mIn, ~0U, &count);
            if (NS_FAILED(rv)) {
                printf("Write failed\n");
                break;
            }
            if (count == 0) {
                printf("EOF count = %d\n", mCount);
                break;
            }

            if (gTrace) {
                printf("Wrote: %d\n", count);
            }
            mCount += count;
        }
        mOut->Close();
        return rv;
    }

    nsPump(nsIInputStream* in,
           nsIOutputStream* out)
        : mIn(in), mOut(out), mCount(0) {
    }

private:
    ~nsPump() {}

protected:
    nsCOMPtr<nsIInputStream>      mIn;
    nsCOMPtr<nsIOutputStream>     mOut;
    PRUint32                            mCount;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsPump,
                              nsIRunnable)

nsresult
TestChainedPipes()
{
    nsresult rv;
    printf("TestChainedPipes\n");

    nsIInputStream* in1;
    nsIOutputStream* out1;
    rv = NS_NewPipe(&in1, &out1, 20, 1999);
    if (NS_FAILED(rv)) return rv;

    nsIInputStream* in2;
    nsIOutputStream* out2;
    rv = NS_NewPipe(&in2, &out2, 200, 401);
    if (NS_FAILED(rv)) return rv;

    nsIThread* thread;
    nsPump* pump = new nsPump(in1, out2);
    if (pump == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(pump);

    rv = NS_NewThread(&thread, pump);
    if (NS_FAILED(rv)) return rv;

    nsIThread* receiverThread;
    nsReceiver* receiver = new nsReceiver(in2);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&receiverThread, receiver);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        PRUint32 len = strlen(buf);
        len = len * rand() / RAND_MAX;
        len = PR_MAX(1, len);
        rv = WriteAll(out1, buf, len, &writeCount);
        if (NS_FAILED(rv)) return rv;
        NS_ASSERTION(writeCount == len, "didn't write enough");
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);

        PR_smprintf_free(buf);
    }
    printf("wrote total of %d bytes\n", total);
    rv = out1->Close();
    if (NS_FAILED(rv)) return rv;

    thread->Shutdown();
    receiverThread->Shutdown();

    NS_RELEASE(thread);
    NS_RELEASE(pump);
    NS_RELEASE(receiverThread);
    NS_RELEASE(receiver);
    return NS_OK;
}



void
RunTests(PRUint32 segSize, PRUint32 segCount)
{
    nsresult rv;
    nsIInputStream* in;
    nsIOutputStream* out;
    PRUint32 bufSize;

    bufSize = segSize * segCount;
    printf("Testing New Pipes: segment size %d buffer size %d\n", segSize, bufSize);

    printf("Testing long writes...\n");
    rv = NS_NewPipe(&in, &out, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestPipe(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");

    printf("Testing short writes...\n");
    rv = NS_NewPipe(&in, &out, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestShortWrites(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");
}


#if 0
void
TestSearch(const char* delim, PRUint32 segSize)
{
    nsresult rv;
    
    PRUint32 bufDataSize = segSize * 2;
    PRUint32 bufSize = segSize * 2;
    nsIInputStream* in;
    nsIOutputStream* out;
    rv = NS_NewPipe(&in, &out, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    out->SetNonBlocking(PR_TRUE);

    PRUint32 i, j, amt;
    PRUint32 delimLen = nsCRT::strlen(delim);
    for (i = 0; i < bufDataSize; i++) {
        
        for (j = 0; j < i; j++) {
            rv = out->Write("-", 1, &amt);
            NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
        }
        rv = out->Write(delim, delimLen, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Write failed");
        if (i + amt < bufDataSize) {
            for (j = i + amt; j < bufDataSize; j++) {
                rv = out->Write("+", 1, &amt);
                NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
            }
        }

        
        PRBool found;
        PRUint32 offset;
        rv = in->Search(delim, PR_FALSE, &found, &offset);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Search failed");

        
        char* bufferContents = new char[bufDataSize + 1];
        rv = in->Read(bufferContents, bufDataSize, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv) && amt == bufDataSize, "Read failed");
        bufferContents[bufDataSize] = '\0';
        printf("Buffer: %s\nDelim: %s %s offset: %d\n", bufferContents,
               delim, (found ? "found" : "not found"), offset);
    }
    NS_RELEASE(in);
    NS_RELEASE(out);
}
#endif


#ifdef DEBUG
extern NS_COM void
TestSegmentedBuffer();
#endif

int
main(int argc, char* argv[])
{
    nsresult rv;
    nsIServiceManager* servMgr;

    rv = NS_InitXPCOM2(&servMgr, NULL, NULL);
    if (NS_FAILED(rv)) return rv;

    if (argc > 1 && nsCRT::strcmp(argv[1], "-trace") == 0)
        gTrace = PR_TRUE;

#ifdef DEBUG
    TestSegmentedBuffer();
#endif

#if 0   
    rv = NS_NewPipe(&in, &out, 4096 * 4);
    if (NS_FAILED(rv)) {
        printf("NewPipe failed\n");
        return -1;
    }

    rv = TestPipe(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    if (NS_FAILED(rv)) {
        printf("TestPipe failed\n");
        return -1;
    }
#endif
#if 0
    TestSearch("foo", 8);
    TestSearch("bar", 6);
    TestSearch("baz", 2);
#endif

    rv = TestChainedPipes();
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestChainedPipes failed");
    RunTests(16, 1);
    RunTests(4096, 16);
    NS_RELEASE(servMgr);
    rv = NS_ShutdownXPCOM( NULL );
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
    return 0;
}


