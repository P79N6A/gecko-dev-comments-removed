





































#ifndef nsSharedPrefHandler_h__
#define nsSharedPrefHandler_h__


#include "ipcITransactionService.h"
#include "ipcITransactionObserver.h"


#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsVoidArray.h"


#include "prefapi.h"


class nsPrefService;





class nsSharedPrefHandler : public ipcITransactionObserver
{
  friend nsresult NS_CreateSharedPrefHandler(nsPrefService*);
    
  NS_DECL_ISUPPORTS
  NS_DECL_IPCITRANSACTIONOBSERVER
    
public:    
  nsresult        OnSessionBegin();
  nsresult        OnSessionEnd();

  nsresult        OnSavePrefs();
  
  nsresult        OnPrefChanged(PRBool defaultPref,
                                PrefHashEntry* pref,
                                PrefValue newValue);

  void            ReadingUserPrefs(PRBool isReading)
                  { mReadingUserPrefs = isReading; }

  PRBool          IsPrefShared(const char* prefName);
  
protected:
                  nsSharedPrefHandler();
  virtual         ~nsSharedPrefHandler();

  nsresult        Init(nsPrefService *aOwner);
  nsresult        ReadExceptionFile();
  nsresult        EnsureTransactionService();
  
protected:
  nsPrefService   *mPrefService;      
  
  nsCOMPtr<ipcITransactionService> mTransService;
  const nsCString mPrefsTSQueueName;
  
  PRPackedBool    mSessionActive;
  PRPackedBool    mReadingUserPrefs;
  PRPackedBool    mProcessingTransaction;
  
  nsAutoVoidArray mExceptionList;
};



extern nsSharedPrefHandler *gSharedPrefHandler; 







extern nsresult NS_CreateSharedPrefHandler(nsPrefService *aOwner);
                                        
#endif 
