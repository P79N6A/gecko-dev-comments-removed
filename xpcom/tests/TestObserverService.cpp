





































#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIEnumerator.h"
#include "nsStringGlue.h"
#include "nsWeakReference.h"
#include "nsComponentManagerUtils.h"

#include <stdio.h>

static nsIObserverService *anObserverService = NULL;

static void testResult( nsresult rv ) {
    if ( NS_SUCCEEDED( rv ) ) {
        printf("...ok\n");
    } else {
        printf("...failed, rv=0x%x\n", (int)rv);
    }
    return;
}

void printString(nsString &str) {
    printf("%s", str.get());
}

class TestObserver : public nsIObserver, public nsSupportsWeakReference {
public:
    TestObserver( const nsAString &name )
        : mName( name ) {
    }
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsString mName;

private:
    ~TestObserver() {}
};

NS_IMPL_ISUPPORTS2( TestObserver, nsIObserver, nsISupportsWeakReference )

NS_IMETHODIMP
TestObserver::Observe( nsISupports     *aSubject,
                       const char *aTopic,
                       const PRUnichar *someData ) {
    nsCString topic( aTopic );
    nsString data( someData );
    	



    printString(mName);
    printf(" has observed something: subject@%p", (void*)aSubject);
    printf(" name=");
    printString(reinterpret_cast<TestObserver*>(reinterpret_cast<void*>(aSubject))->mName);
    printf(" aTopic=%s", topic.get());
    printf(" someData=");
    printString(data);
    printf("\n");
    return NS_OK;
}

int main(int argc, char *argv[])
{
    nsCString topicA; topicA.Assign( "topic-A" );
    nsCString topicB; topicB.Assign( "topic-B" );
    nsresult rv;

    nsresult res = CallCreateInstance("@mozilla.org/observer-service;1", &anObserverService);
	
    if (res == NS_OK) {

        nsIObserver *aObserver = new TestObserver(NS_LITERAL_STRING("Observer-A"));
        aObserver->AddRef();
        nsIObserver *bObserver = new TestObserver(NS_LITERAL_STRING("Observer-B"));
        bObserver->AddRef();
            
        printf("Adding Observer-A as observer of topic-A...\n");
        rv = anObserverService->AddObserver(aObserver, topicA.get(), PR_FALSE);
        testResult(rv);
 
        printf("Adding Observer-B as observer of topic-A...\n");
        rv = anObserverService->AddObserver(bObserver, topicA.get(), PR_FALSE);
        testResult(rv);
 
        printf("Adding Observer-B as observer of topic-B...\n");
        rv = anObserverService->AddObserver(bObserver, topicB.get(), PR_FALSE);
        testResult(rv);

        printf("Testing Notify(observer-A, topic-A)...\n");
        rv = anObserverService->NotifyObservers( aObserver,
                                   topicA.get(),
                                   NS_LITERAL_STRING("Testing Notify(observer-A, topic-A)").get() );
        testResult(rv);

        printf("Testing Notify(observer-B, topic-B)...\n");
        rv = anObserverService->NotifyObservers( bObserver,
                                   topicB.get(),
                                   NS_LITERAL_STRING("Testing Notify(observer-B, topic-B)").get() );
        testResult(rv);
 
        printf("Testing EnumerateObserverList (for topic-A)...\n");
        nsCOMPtr<nsISimpleEnumerator> e;
        rv = anObserverService->EnumerateObservers(topicA.get(), getter_AddRefs(e));

        testResult(rv);

        printf("Enumerating observers of topic-A...\n");
        if ( e ) {
          nsCOMPtr<nsIObserver> observer;
          PRBool loop = PR_TRUE;
          while( NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) 
          {
              e->GetNext(getter_AddRefs(observer));
              printf("Calling observe on enumerated observer ");
              printString(reinterpret_cast<TestObserver*>
                                          (reinterpret_cast<void*>(observer.get()))->mName);
              printf("...\n");
              rv = observer->Observe( observer, 
                                      topicA.get(), 
                                      NS_LITERAL_STRING("during enumeration").get() );
              testResult(rv);
          }
        }
        printf("...done enumerating observers of topic-A\n");

        printf("Removing Observer-A...\n");
        rv = anObserverService->RemoveObserver(aObserver, topicA.get());
        testResult(rv);


        printf("Removing Observer-B (topic-A)...\n");
        rv = anObserverService->RemoveObserver(bObserver, topicB.get());
        testResult(rv);
        printf("Removing Observer-B (topic-B)...\n");
        rv = anObserverService->RemoveObserver(bObserver, topicA.get());
        testResult(rv);
       
    }
    return NS_OK;
}
