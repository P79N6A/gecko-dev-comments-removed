




































#include "TestCommon.h"
#include "nsNetUtil.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsIComponentManager.h"
#include "prprf.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "prlong.h"
#include "plstr.h"
#include "nsCOMArray.h"
#include "nsIComponentRegistrar.h"

namespace TestPageLoad {

int getStrLine(const char *src, char *str, int ind, int max);
nsresult auxLoad(char *uriBuf);



#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf(">>> %s failed: rv=%x\n", step, rv); \
        return rv;\
    } \
    PR_END_MACRO

static nsCString globalStream;

static nsCOMPtr<nsIURI> baseURI;
static nsCOMArray<nsIURI> uriList;


static int numStart=0;
static int numFound=0;

static PRInt32 gKeepRunning = 0;




static NS_METHOD streamParse (nsIInputStream* in,
                              void* closure,
                              const char* fromRawSegment,
                              PRUint32 toOffset,
                              PRUint32 count,
                              PRUint32 *writeCount) {

  char parseBuf[2048], loc[2048], lineBuf[2048];
  char *loc_t, *loc_t2;
  int i = 0;
  const char *tmp;

  if(!globalStream.IsEmpty()) {
    globalStream.Append(fromRawSegment);
    tmp = globalStream.get();
    
  } else {
    tmp = fromRawSegment;
  }

  while(i < (int)count) {
    i = getStrLine(tmp, lineBuf, i, count);
    if(i < 0) {
      *writeCount = count;
      return NS_OK;
    }
    parseBuf[0]='\0';
    if((loc_t=PL_strcasestr(lineBuf, "img"))!= NULL 
       || (loc_t=PL_strcasestr(lineBuf, "script"))!=NULL) {
      loc_t2=PL_strcasestr(loc_t, "src");
      if(loc_t2!=NULL) {
        loc_t2+=3;
        strcpy(loc, loc_t2);
        sscanf(loc, "=\"%[^\"]", parseBuf);
        if(parseBuf[0]=='\0')
          sscanf(loc, "=%s", parseBuf);         
        if(parseBuf[0]!='\0'){
          numFound++;
          auxLoad(parseBuf);
        }     
      }
    }

    















    if((loc_t=PL_strcasestr(lineBuf, "background"))!=NULL) {
      loc_t+=10;
      strcpy(loc, loc_t);
      sscanf(loc, "=\"%[^\"]", parseBuf);
      if(parseBuf[0]!='\0') {
        numFound++;
        auxLoad(parseBuf);
      }
    }
    i++;

  }
  *writeCount = count;
  return NS_OK;
}





class MyListener : public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    MyListener() { }
    virtual ~MyListener() {}
};

NS_IMPL_ISUPPORTS2(MyListener,
                   nsIRequestObserver,
                   nsIStreamListener)

NS_IMETHODIMP
MyListener::OnStartRequest(nsIRequest *req, nsISupports *ctxt)
{
  
    numStart++;
    return NS_OK;
}

NS_IMETHODIMP
MyListener::OnStopRequest(nsIRequest *req, nsISupports *ctxt, nsresult status)
{
    
    if (--gKeepRunning == 0)
      QuitPumpingEvents();
    return NS_OK;
}

NS_IMETHODIMP
MyListener::OnDataAvailable(nsIRequest *req, nsISupports *ctxt,
                            nsIInputStream *stream,
                            PRUint32 offset, PRUint32 count)
{
    
    nsresult rv = NS_ERROR_FAILURE;
    PRUint32 bytesRead=0;
    char buf[1024];

    if(ctxt == nsnull) {
      bytesRead=0;
      rv = stream->ReadSegments(streamParse, &offset, count, &bytesRead);
    } else {
      while (count) {
        PRUint32 amount = NS_MIN<PRUint32>(count, sizeof(buf));
        rv = stream->Read(buf, amount, &bytesRead);  
        count -= bytesRead;
      }
    }

    if (NS_FAILED(rv)) {
      printf(">>> stream->Read failed with rv=%x\n", rv);
      return rv;
    }

    return NS_OK;
}





class MyNotifications : public nsIInterfaceRequestor
                      , public nsIProgressEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK

    MyNotifications() { }
    virtual ~MyNotifications() {}
};

NS_IMPL_THREADSAFE_ISUPPORTS2(MyNotifications,
                              nsIInterfaceRequestor,
                              nsIProgressEventSink)

NS_IMETHODIMP
MyNotifications::GetInterface(const nsIID &iid, void **result)
{
    return QueryInterface(iid, result);
}

NS_IMETHODIMP
MyNotifications::OnStatus(nsIRequest *req, nsISupports *ctx,
                          nsresult status, const PRUnichar *statusText)
{
    
    return NS_OK;
}

NS_IMETHODIMP
MyNotifications::OnProgress(nsIRequest *req, nsISupports *ctx,
                            PRUint64 progress, PRUint64 progressMax)
{
    
    
    
    return NS_OK;
}











int getStrLine(const char *src, char *str, int ind, int max) {
  char c = src[ind];
  int i=0;
  globalStream.Assign('\0');
  while(c!='\n' && c!='\0' && i<max) {
    str[i] = src[ind];
    i++; ind++;
    c = src[ind];
  }
  str[i]='\0';
  if(i==max || c=='\0') {
    globalStream.Assign(str);
    
    return -1;
  }
  return ind;
}


nsresult auxLoad(char *uriBuf)
{
    nsresult rv;

    nsCOMPtr<nsISupportsPRBool> myBool = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);

    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIChannel> chan;
    nsCOMPtr<nsIStreamListener> listener = new MyListener();
    nsCOMPtr<nsIInterfaceRequestor> callbacks = new MyNotifications();

    printf("Getting: %s", uriBuf);

    
    if(strncmp(uriBuf, "http:", 5)) {
      
      rv = NS_NewURI(getter_AddRefs(uri), uriBuf, baseURI);
      if (NS_FAILED(rv)) return(rv);
    } else {
      
      rv = NS_NewURI(getter_AddRefs(uri), uriBuf);
      if (NS_FAILED(rv)) return(rv);
    }

    
    PRBool equal;
    for(PRInt32 i = 0; i < uriList.Count(); i++) {
      uri->Equals(uriList[i], &equal);
      if(equal) {
        printf("(duplicate, canceling) %s\n",uriBuf); 
        return NS_OK;
      }
    }
    printf("\n");
    uriList.AppendObject(uri);
    rv = NS_NewChannel(getter_AddRefs(chan), uri, nsnull, nsnull, callbacks);
    RETURN_IF_FAILED(rv, "NS_NewChannel");

    gKeepRunning++;
    rv = chan->AsyncOpen(listener, myBool);
    RETURN_IF_FAILED(rv, "AsyncOpen");

    return NS_OK;

}



} 

using namespace TestPageLoad;



int main(int argc, char **argv)
{ 
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    nsresult rv;

    if (argc == 1) {
        printf("usage: TestPageLoad <url>\n");
        return -1;
    }
    {
        nsCOMPtr<nsIServiceManager> servMan;
        NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);

        PRTime start, finish;

        printf("Loading necko ... \n");
        nsCOMPtr<nsIChannel> chan;
        nsCOMPtr<nsIStreamListener> listener = new MyListener();
        nsCOMPtr<nsIInterfaceRequestor> callbacks = new MyNotifications();

        rv = NS_NewURI(getter_AddRefs(baseURI), argv[1]);
        RETURN_IF_FAILED(rv, "NS_NewURI");

        rv = NS_NewChannel(getter_AddRefs(chan), baseURI, nsnull, nsnull, callbacks);
        RETURN_IF_FAILED(rv, "NS_OpenURI");
        gKeepRunning++;

        
        printf("Starting clock ... \n");
        start = PR_Now();
        rv = chan->AsyncOpen(listener, nsnull);
        RETURN_IF_FAILED(rv, "AsyncOpen");

        PumpEvents();

        finish = PR_Now();
        PRUint32 totalTime32;
        PRUint64 totalTime64;
        LL_SUB(totalTime64, finish, start);
        LL_L2UI(totalTime32, totalTime64);

        printf("\n\n--------------------\nAll done:\nnum found:%d\nnum start:%d\n", numFound, numStart);

        printf("\n\n>>PageLoadTime>>%u>>\n\n", totalTime32);
    } 
    
    rv = NS_ShutdownXPCOM(nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
    return 0;
}
