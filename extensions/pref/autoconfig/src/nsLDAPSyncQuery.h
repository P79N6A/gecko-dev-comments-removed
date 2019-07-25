






































#include "nsCOMPtr.h"
#include "nsILDAPConnection.h"
#include "nsILDAPOperation.h"
#include "nsILDAPMessageListener.h"
#include "nsILDAPURL.h"
#include "nsString.h"
#include "nsILDAPSyncQuery.h"


#define NS_LDAPSYNCQUERY_CID \
{ 0xdddee14e, 0xed81, 0x4182, \
 { 0x93, 0x23, 0xc2, 0xab, 0x22, 0xfb, 0xa6, 0x8e }}


class nsLDAPSyncQuery : public nsILDAPSyncQuery,
                        public nsILDAPMessageListener

{
  public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSILDAPMESSAGELISTENER
    NS_DECL_NSILDAPSYNCQUERY

    nsLDAPSyncQuery();
    virtual ~nsLDAPSyncQuery();

  protected:

    nsCOMPtr<nsILDAPConnection> mConnection; 
    nsCOMPtr<nsILDAPOperation> mOperation;   
    nsCOMPtr<nsILDAPURL> mServerURL;         
    PRBool mFinished;                        
    PRUint32 mAttrCount;                     
    char **mAttrs;                           
    nsString mResults;                       
    PRUint32 mProtocolVersion;               

    nsresult InitConnection();
    
    nsresult OnLDAPBind(nsILDAPMessage *aMessage); 

    
    nsresult OnLDAPSearchEntry(nsILDAPMessage *aMessage); 


    nsresult OnLDAPSearchResult(nsILDAPMessage *aMessage); 

    
    nsresult StartLDAPSearch();
    
    
    void FinishLDAPQuery();
};

