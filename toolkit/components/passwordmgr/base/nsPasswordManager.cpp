





































#include "nsPasswordManager.h"
#include "nsSingleSignonPrompt.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsILineInputStream.h"
#include "plbase64.h"
#include "nsISecretDecoderRing.h"
#include "nsIPasswordInternal.h"
#include "nsIPrompt.h"
#include "nsIPromptService2.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "prmem.h"
#include "nsIStringBundle.h"
#include "nsIMutableArray.h"
#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsIWebProgress.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIForm.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIContent.h"
#include "nsIFormControl.h"
#include "nsIDOMWindowInternal.h"
#include "nsCURILoader.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIAutoCompleteResult.h"
#include "nsIPK11TokenDB.h"
#include "nsIPK11Token.h"
#include "nsUnicharUtils.h"
#include "nsCOMArray.h"
#include "nsEmbedCID.h"

static const char kPMPropertiesURL[] = "chrome://passwordmgr/locale/passwordmgr.properties";
static PRBool sRememberPasswords = PR_FALSE;
static PRBool sPrefsInitialized = PR_FALSE;
static PRBool sPasswordsLoaded = PR_FALSE;

static nsIStringBundle* sPMBundle;
static nsISecretDecoderRing* sDecoderRing;
static nsPasswordManager* sPasswordManager;

class nsPasswordManager::SignonDataEntry
{
public:
  nsString userField;
  nsString userValue;
  nsString passField;
  nsString passValue;
  nsCString actionOrigin;
  SignonDataEntry* next;

  SignonDataEntry() : next(nsnull) { }
  ~SignonDataEntry() { delete next; }
};

class nsPasswordManager::SignonHashEntry
{
  
  
  

public:
  SignonDataEntry* head;

  SignonHashEntry(SignonDataEntry* aEntry) : head(aEntry) { }
  ~SignonHashEntry() { delete head; }
};

class nsPasswordManager::PasswordEntry : public nsIPasswordInternal
{
public:
  PasswordEntry(const nsACString& aKey, SignonDataEntry* aData);
  virtual ~PasswordEntry() { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPASSWORD
  NS_DECL_NSIPASSWORDINTERNAL

protected:

  nsCString mHost;
  nsString  mUser;
  nsString  mUserField;
  nsString  mPassword;
  nsString  mPasswordField;
  nsCString mActionOrigin;
  PRBool    mDecrypted[2];
};

NS_IMPL_ISUPPORTS2(nsPasswordManager::PasswordEntry, nsIPassword,
                   nsIPasswordInternal)

nsPasswordManager::PasswordEntry::PasswordEntry(const nsACString& aKey,
                                                SignonDataEntry* aData)
  : mHost(aKey)
{
    mDecrypted[0] = mDecrypted[1] = PR_FALSE;

  if (aData) {
    mUser.Assign(aData->userValue);
    mUserField.Assign(aData->userField);
    mPassword.Assign(aData->passValue);
    mPasswordField.Assign(aData->passField);
    mActionOrigin.Assign(aData->actionOrigin);
  }
}

NS_IMETHODIMP
nsPasswordManager::PasswordEntry::GetHost(nsACString& aHost)
{
  aHost.Assign(mHost);
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::PasswordEntry::GetUser(nsAString& aUser)
{
  if (!mUser.IsEmpty() && !mDecrypted[0]) {
    if (NS_SUCCEEDED(DecryptData(mUser, mUser)))
      mDecrypted[0] = PR_TRUE;
    else
      return NS_ERROR_FAILURE;
  }

  aUser.Assign(mUser);
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::PasswordEntry::GetPassword(nsAString& aPassword)
{
  if (!mPassword.IsEmpty() && !mDecrypted[1]) {
    if (NS_SUCCEEDED(DecryptData(mPassword, mPassword)))
      mDecrypted[1] = PR_TRUE;
    else
      return NS_ERROR_FAILURE;
  }

  aPassword.Assign(mPassword);
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::PasswordEntry::GetUserFieldName(nsAString& aField)
{
  aField.Assign(mUserField);
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::PasswordEntry::GetPasswordFieldName(nsAString& aField)
{
  aField.Assign(mPasswordField);
  return NS_OK;
}



NS_IMPL_ADDREF(nsPasswordManager)
NS_IMPL_RELEASE(nsPasswordManager)

NS_INTERFACE_MAP_BEGIN(nsPasswordManager)
  NS_INTERFACE_MAP_ENTRY(nsIPasswordManager)
  NS_INTERFACE_MAP_ENTRY(nsIPasswordManagerInternal)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIFormSubmitObserver)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPasswordManager)
  NS_INTERFACE_MAP_ENTRY(nsIPromptFactory)
NS_INTERFACE_MAP_END

nsPasswordManager::nsPasswordManager()
  : mAutoCompletingField(nsnull)
{
}

nsPasswordManager::~nsPasswordManager()
{
}


 nsPasswordManager*
nsPasswordManager::GetInstance()
{
  if (!sPasswordManager) {
    sPasswordManager = new nsPasswordManager();
    if (!sPasswordManager)
      return nsnull;

    NS_ADDREF(sPasswordManager);   

    if (NS_FAILED(sPasswordManager->Init())) {
      NS_RELEASE(sPasswordManager);
      return nsnull;
    }
  }

  
  
  
  sPasswordManager->LoadPasswords();

  NS_ADDREF(sPasswordManager);   
  return sPasswordManager;
}

nsresult
nsPasswordManager::Init()
{
  mSignonTable.Init();
  mRejectTable.Init();
  mAutoCompleteInputs.Init();

  sPrefsInitialized = PR_TRUE;

  nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ASSERTION(prefService, "No pref service");

  prefService->GetBranch("signon.", getter_AddRefs(mPrefBranch));
  NS_ASSERTION(mPrefBranch, "No pref branch");

  mPrefBranch->GetBoolPref("rememberSignons", &sRememberPasswords);

  nsCOMPtr<nsIPrefBranch2> branchInternal = do_QueryInterface(mPrefBranch);

  
  

  branchInternal->AddObserver("rememberSignons", this, PR_TRUE);


  
  

  nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(obsService, "No observer service");

  obsService->AddObserver(this, NS_EARLYFORMSUBMIT_SUBJECT, PR_TRUE);

  nsCOMPtr<nsIWebProgress> progress = do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
  NS_ASSERTION(progress, "No web progress service");

  progress->AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_DOCUMENT);
  return NS_OK;
}

 PRBool
nsPasswordManager::SingleSignonEnabled()
{
  if (!sPrefsInitialized) {
    
    nsCOMPtr<nsIPasswordManager> manager = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
  }

  return sRememberPasswords;
}

 NS_METHOD
nsPasswordManager::Register(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* aRegistryLocation,
                            const char* aComponentType,
                            const nsModuleComponentInfo* aInfo)
{
  
  
  

  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString prevEntry;
  catman->AddCategoryEntry(NS_PASSWORDMANAGER_CATEGORY,
                           "Password Manager",
                           NS_PASSWORDMANAGER_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           getter_Copies(prevEntry));

  catman->AddCategoryEntry("app-startup",
                           "Password Manager",
                           NS_PASSWORDMANAGER_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           getter_Copies(prevEntry));

  return NS_OK;
}

 NS_METHOD
nsPasswordManager::Unregister(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* aRegistryLocation,
                              const nsModuleComponentInfo* aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  catman->DeleteCategoryEntry(NS_PASSWORDMANAGER_CATEGORY,
                              NS_PASSWORDMANAGER_CONTRACTID,
                              PR_TRUE);

  return NS_OK;
}

 void
nsPasswordManager::Shutdown()
{
  NS_IF_RELEASE(sDecoderRing);
  NS_IF_RELEASE(sPMBundle);
  NS_IF_RELEASE(sPasswordManager);
}



NS_IMETHODIMP
nsPasswordManager::AddUser(const nsACString& aHost,
                           const nsAString& aUser,
                           const nsAString& aPassword)
{
  
  
  if (aUser.IsEmpty() && aPassword.IsEmpty())
    return NS_OK;

  
  if (!aHost.IsEmpty()) {
    SignonHashEntry *hashEnt;
    if (mSignonTable.Get(aHost, &hashEnt)) {
      nsString empty;
      SignonDataEntry *entry = nsnull;
      FindPasswordEntryInternal(hashEnt->head, aUser, empty, empty, &entry);
      if (entry) {
        
        return EncryptDataUCS2(aPassword, entry->passValue);
      }
    }
  }

  SignonDataEntry* entry = new SignonDataEntry();
  if (NS_FAILED(EncryptDataUCS2(aUser, entry->userValue)) ||
      NS_FAILED(EncryptDataUCS2(aPassword, entry->passValue))) {
    delete entry;
    return NS_ERROR_FAILURE;
  }

  AddSignonData(aHost, entry);
  WritePasswords(mSignonFile);

  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::RemoveUser(const nsACString& aHost, const nsAString& aUser)
{
  SignonDataEntry* entry, *prevEntry = nsnull;
  SignonHashEntry* hashEnt;

  if (!mSignonTable.Get(aHost, &hashEnt))
    return NS_ERROR_FAILURE;

  for (entry = hashEnt->head; entry; prevEntry = entry, entry = entry->next) {

    nsAutoString ptUser;
    if (!entry->userValue.IsEmpty() &&
        NS_FAILED(DecryptData(entry->userValue, ptUser)))
      break;

    if (ptUser.Equals(aUser)) {
      if (prevEntry)
        prevEntry->next = entry->next;
      else
        hashEnt->head = entry->next;

      entry->next = nsnull;
      delete entry;

      if (!hashEnt->head)
        mSignonTable.Remove(aHost);  

      WritePasswords(mSignonFile);

      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPasswordManager::AddReject(const nsACString& aHost)
{
  mRejectTable.Put(aHost, 1);
  WritePasswords(mSignonFile);
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::RemoveReject(const nsACString& aHost)
{
  mRejectTable.Remove(aHost);
  WritePasswords(mSignonFile);
  return NS_OK;
}

 PLDHashOperator PR_CALLBACK
nsPasswordManager::BuildArrayEnumerator(const nsACString& aKey,
                                        SignonHashEntry* aEntry,
                                        void* aUserData)
{
  nsIMutableArray* array = NS_STATIC_CAST(nsIMutableArray*, aUserData);

  for (SignonDataEntry* e = aEntry->head; e; e = e->next)
    array->AppendElement(new PasswordEntry(aKey, e), PR_FALSE);

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsPasswordManager::GetEnumerator(nsISimpleEnumerator** aEnumerator)
{
  
  nsCOMPtr<nsIMutableArray> signonArray =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(signonArray);

  mSignonTable.EnumerateRead(BuildArrayEnumerator, signonArray);

  return signonArray->Enumerate(aEnumerator);
}

 PLDHashOperator PR_CALLBACK
nsPasswordManager::BuildRejectArrayEnumerator(const nsACString& aKey,
                                              PRInt32 aEntry,
                                              void* aUserData)
{
  nsIMutableArray* array = NS_STATIC_CAST(nsIMutableArray*, aUserData);

  nsCOMPtr<nsIPassword> passwordEntry = new PasswordEntry(aKey, nsnull);
  array->AppendElement(passwordEntry, PR_FALSE);

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsPasswordManager::GetRejectEnumerator(nsISimpleEnumerator** aEnumerator)
{
  
  nsCOMPtr<nsIMutableArray> rejectArray =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(rejectArray);

  mRejectTable.EnumerateRead(BuildRejectArrayEnumerator, rejectArray);

  return rejectArray->Enumerate(aEnumerator);
}



struct findEntryContext {
  nsPasswordManager* manager;
  const nsACString& hostURI;
  const nsAString&  username;
  const nsAString&  password;
  nsACString& hostURIFound;
  nsAString&  usernameFound;
  nsAString&  passwordFound;
  PRBool matched;

  findEntryContext(nsPasswordManager* aManager,
                   const nsACString& aHostURI,
                   const nsAString&  aUsername,
                   const nsAString&  aPassword,
                   nsACString& aHostURIFound,
                   nsAString&  aUsernameFound,
                   nsAString&  aPasswordFound)
    : manager(aManager), hostURI(aHostURI), username(aUsername),
      password(aPassword), hostURIFound(aHostURIFound),
      usernameFound(aUsernameFound), passwordFound(aPasswordFound),
      matched(PR_FALSE) { }
};

 PLDHashOperator PR_CALLBACK
nsPasswordManager::FindEntryEnumerator(const nsACString& aKey,
                                       SignonHashEntry* aEntry,
                                       void* aUserData)
{
  findEntryContext* context = NS_STATIC_CAST(findEntryContext*, aUserData);
  nsPasswordManager* manager = context->manager;
  nsresult rv;

  SignonDataEntry* entry = nsnull;
  rv = manager->FindPasswordEntryInternal(aEntry->head,
                                          context->username,
                                          context->password,
                                          EmptyString(),
                                          &entry);

  if (NS_SUCCEEDED(rv) && entry) {
    if (NS_SUCCEEDED(DecryptData(entry->userValue, context->usernameFound)) &&
        NS_SUCCEEDED(DecryptData(entry->passValue, context->passwordFound))) {
      context->matched = PR_TRUE;
      context->hostURIFound.Assign(context->hostURI);
    }

    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsPasswordManager::FindPasswordEntry(const nsACString& aHostURI,
                                     const nsAString&  aUsername,
                                     const nsAString&  aPassword,
                                     nsACString& aHostURIFound,
                                     nsAString&  aUsernameFound,
                                     nsAString&  aPasswordFound)
{
  if (!aHostURI.IsEmpty()) {
    SignonHashEntry* hashEnt;
    if (mSignonTable.Get(aHostURI, &hashEnt)) {
      SignonDataEntry* entry;
      nsresult rv = FindPasswordEntryInternal(hashEnt->head,
                                              aUsername,
                                              aPassword,
                                              EmptyString(),
                                              &entry);

      if (NS_SUCCEEDED(rv) && entry) {
        if (NS_SUCCEEDED(DecryptData(entry->userValue, aUsernameFound)) &&
            NS_SUCCEEDED(DecryptData(entry->passValue, aPasswordFound))) {
          aHostURIFound.Assign(aHostURI);
        } else {
          return NS_ERROR_FAILURE;
        }
      }

      return rv;
    }

    return NS_ERROR_FAILURE;
  }

  
  findEntryContext context(this, aHostURI, aUsername, aPassword,
                           aHostURIFound, aUsernameFound, aPasswordFound);

  mSignonTable.EnumerateRead(FindEntryEnumerator, &context);

  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::AddUserFull(const nsACString& aKey,
                               const nsAString& aUser,
                               const nsAString& aPassword,
                               const nsAString& aUserFieldName,
                               const nsAString& aPassFieldName)
{
  
  
  if (aUser.IsEmpty() && aPassword.IsEmpty())
    return NS_OK;

  
  if (!aKey.IsEmpty()) {
    SignonHashEntry *hashEnt;
    if (mSignonTable.Get(aKey, &hashEnt)) {
      nsString empty;
      SignonDataEntry *entry = nsnull;
      FindPasswordEntryInternal(hashEnt->head, aUser, empty, empty, &entry);
      if (entry) {
        
        EncryptDataUCS2(aPassword, entry->passValue);
        
        entry->userField.Assign(aUserFieldName);
        entry->passField.Assign(aPassFieldName);
        return NS_OK;
      }
    }
  }

  SignonDataEntry* entry = new SignonDataEntry();
  entry->userField.Assign(aUserFieldName);
  entry->passField.Assign(aPassFieldName);
  EncryptDataUCS2(aUser, entry->userValue);
  EncryptDataUCS2(aPassword, entry->passValue);

  AddSignonData(aKey, entry);
  WritePasswords(mSignonFile);

  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::ReadPasswords(nsIFile* aPasswordFile)
{
  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), aPasswordFile);
  if (!fileStream)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream);
  NS_ASSERTION(lineStream, "File stream is not an nsILineInputStream");

  
  nsCAutoString utf8Buffer;
  PRBool moreData = PR_FALSE;
  nsresult rv = lineStream->ReadLine(utf8Buffer, &moreData);
  if (NS_FAILED(rv))
    return NS_OK;

  PRBool updateEntries = PR_FALSE;
  PRBool writeOnFinish = PR_FALSE;

  if (utf8Buffer.Equals("#2c")) {
    
    updateEntries = PR_TRUE;

    
    writeOnFinish = PR_TRUE;
  } else if (!utf8Buffer.Equals("#2d")) {
    NS_ERROR("Unexpected version header in signon file");
    return NS_OK;
  }

  enum { STATE_REJECT, STATE_REALM, STATE_USERFIELD,
         STATE_USERVALUE, STATE_PASSFIELD, STATE_PASSVALUE,
         STATE_ACTION_ORIGIN } state = STATE_REJECT;

  nsCAutoString realm;
  SignonDataEntry* entry = nsnull;

  do {
    rv = lineStream->ReadLine(utf8Buffer, &moreData);
    if (NS_FAILED(rv))
      return NS_OK;

    switch (state) {
    case STATE_REJECT:
      if (utf8Buffer.Equals(NS_LITERAL_CSTRING(".")))
        state = STATE_REALM;
      else
        mRejectTable.Put(utf8Buffer, 1);

      break;

    case STATE_REALM:
      realm.Assign(utf8Buffer);
      state = STATE_USERFIELD;
      break;

    case STATE_USERFIELD:

      
      if (entry) {
        
        if (entry->userValue.IsEmpty() && entry->passValue.IsEmpty()) {
          NS_WARNING("Discarding empty password entry");
          writeOnFinish = PR_TRUE; 
          delete entry;
        } else {
          AddSignonData(realm, entry);
        }
      }

      
      if (utf8Buffer.Equals(NS_LITERAL_CSTRING("."))) {
        entry = nsnull;
        state = STATE_REALM;
      } else {
        entry = new SignonDataEntry();
        CopyUTF8toUTF16(utf8Buffer, entry->userField);
        state = STATE_USERVALUE;
      }

      break;

    case STATE_USERVALUE:
      NS_ASSERTION(entry, "bad state");

      CopyUTF8toUTF16(utf8Buffer, entry->userValue);

      state = STATE_PASSFIELD;
      break;

    case STATE_PASSFIELD:
      NS_ASSERTION(entry, "bad state");

      
      CopyUTF8toUTF16(Substring(utf8Buffer, 1, utf8Buffer.Length() - 1),
                      entry->passField);

      state = STATE_PASSVALUE;
      break;

    case STATE_PASSVALUE:
      NS_ASSERTION(entry, "bad state");

      CopyUTF8toUTF16(utf8Buffer, entry->passValue);

      
      if (updateEntries)
        state = STATE_USERFIELD;
      else
        state = STATE_ACTION_ORIGIN;
      break;

    case STATE_ACTION_ORIGIN:
      NS_ASSERTION(entry, "bad state");

      entry->actionOrigin.Assign(utf8Buffer);

      state = STATE_USERFIELD;

      break;
    }
  } while (moreData);

  
  delete entry;

  if (writeOnFinish) {
    fileStream->Close();
    WritePasswords(mSignonFile);
  }

  return NS_OK;
}



NS_IMETHODIMP
nsPasswordManager::Observe(nsISupports* aSubject,
                           const char* aTopic,
                           const PRUnichar* aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(aSubject);
    NS_ASSERTION(branch == mPrefBranch, "unexpected pref change notification");

    branch->GetBoolPref("rememberSignons", &sRememberPasswords);
  } else if (!strcmp(aTopic, "app-startup")) {
    nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1");
    NS_ASSERTION(obsService, "No observer service");

    obsService->AddObserver(this, "profile-after-change", PR_TRUE);
  } else if (!strcmp(aTopic, "profile-after-change"))
    nsCOMPtr<nsIPasswordManager> pm = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);

  return NS_OK;
}


NS_IMETHODIMP
nsPasswordManager::OnStateChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 PRUint32 aStateFlags,
                                 nsresult aStatus)
{
  if (!(aStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) ||
      !(aStateFlags & nsIWebProgressListener::STATE_TRANSFERRING) ||
      NS_FAILED(aStatus))
    return NS_OK;

  
  if (!SingleSignonEnabled())
    return NS_OK;

  nsCOMPtr<nsIDOMWindow> domWin;
  nsresult rv = aWebProgress->GetDOMWindow(getter_AddRefs(domWin));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc;
  domWin->GetDocument(getter_AddRefs(domDoc));
  NS_ASSERTION(domDoc, "DOM window should always have a document!");

  
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(domDoc);
  if (!htmlDoc)
    return NS_OK;

  if (aStateFlags & nsIWebProgressListener::STATE_RESTORING)
      return FillDocument(domDoc);

  nsCOMPtr<nsIDOMEventTarget> targDoc = do_QueryInterface(domDoc);
  nsCOMPtr<nsIDOMEventTarget> targWin = do_QueryInterface(domWin);
  nsIDOMEventListener* listener = NS_STATIC_CAST(nsIDOMFocusListener*, this);
  targDoc->AddEventListener(NS_LITERAL_STRING("DOMContentLoaded"), listener, PR_FALSE);
  targWin->AddEventListener(NS_LITERAL_STRING("pagehide"), listener, PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::OnProgressChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRInt32 aCurSelfProgress,
                                    PRInt32 aMaxSelfProgress,
                                    PRInt32 aCurTotalProgress,
                                    PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI* aLocation)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::OnSecurityChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRUint32 aState)
{
  return NS_OK;
}



NS_IMETHODIMP
nsPasswordManager::Notify(nsIDOMHTMLFormElement* aDOMForm,
                          nsIDOMWindowInternal* aWindow,
                          nsIURI* aActionURL,
                          PRBool* aCancelSubmit)
{
  nsCOMPtr<nsIContent> formNode = do_QueryInterface(aDOMForm);
  
  

  NS_ENSURE_TRUE(aWindow, NS_OK);

  
  if (!SingleSignonEnabled())
    return NS_OK;

  
  nsCAutoString realm;
  
  
  if (!GetPasswordRealm(formNode->GetOwnerDoc()->GetDocumentURI(), realm))
    return NS_OK;

  PRInt32 rejectValue;
  if (mRejectTable.Get(realm, &rejectValue)) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIForm> formElement = do_QueryInterface(formNode);

  PRUint32 numControls;
  formElement->GetElementCount(&numControls);

  

  nsCOMPtr<nsIDOMHTMLInputElement> userField;
  nsCOMArray<nsIDOMHTMLInputElement> passFields;

  PRUint32 i, firstPasswordIndex = numControls;

  for (i = 0; i < numControls; ++i) {

    nsCOMPtr<nsIFormControl> control;
    formElement->GetElementAt(i, getter_AddRefs(control));

    if (control->GetType() == NS_FORM_INPUT_PASSWORD) {
      nsCOMPtr<nsIDOMHTMLInputElement> elem = do_QueryInterface(control);
      passFields.AppendObject(elem);
      if (firstPasswordIndex == numControls)
        firstPasswordIndex = i;
    }
  }

  nsCOMPtr<nsIPrompt> prompt;
  aWindow->GetPrompter(getter_AddRefs(prompt));

  switch (passFields.Count()) {
  case 1:  
    {
      
      for (PRInt32 j = (PRInt32) firstPasswordIndex - 1; j >= 0; --j) {
        nsCOMPtr<nsIFormControl> control;
        formElement->GetElementAt(j, getter_AddRefs(control));

        if (control->GetType() == NS_FORM_INPUT_TEXT) {
          userField = do_QueryInterface(control);
          break;
        }
      }

      
      

      nsAutoString autocomplete;

      if (userField) {
        nsCOMPtr<nsIDOMElement> userFieldElement = do_QueryInterface(userField);
        userFieldElement->GetAttribute(NS_LITERAL_STRING("autocomplete"),
                                       autocomplete);

        if (autocomplete.EqualsIgnoreCase("off"))
          return NS_OK;
      }

      aDOMForm->GetAttribute(NS_LITERAL_STRING("autocomplete"), autocomplete);
      if (autocomplete.EqualsIgnoreCase("off"))
        return NS_OK;

      nsCOMPtr<nsIDOMElement> passFieldElement = do_QueryInterface(passFields.ObjectAt(0));
      passFieldElement->GetAttribute(NS_LITERAL_STRING("autocomplete"), autocomplete);
      if (autocomplete.EqualsIgnoreCase("off"))
        return NS_OK;


      
      
      

      nsAutoString userValue, passValue, userFieldName, passFieldName, actionOrigin;

      if (userField) {
        userField->GetValue(userValue);
        userField->GetName(userFieldName);
      }

      passFields.ObjectAt(0)->GetValue(passValue);
      passFields.ObjectAt(0)->GetName(passFieldName);

      
      if (passValue.IsEmpty())
        return NS_OK;

      SignonHashEntry* hashEnt;
      nsCAutoString formActionOrigin;

      if (mSignonTable.Get(realm, &hashEnt)) {

        SignonDataEntry* entry;
        nsAutoString buffer;

        for (entry = hashEnt->head; entry; entry = entry->next) {
          if (entry->userField.Equals(userFieldName) &&
              entry->passField.Equals(passFieldName)) {

            if (NS_FAILED(DecryptData(entry->userValue, buffer)))
              return NS_OK;

            if (buffer.Equals(userValue)) {

              if (NS_FAILED(DecryptData(entry->passValue, buffer)))
                return NS_OK;

              PRBool writePasswords = PR_FALSE;
              
              if (!buffer.Equals(passValue)) {
                if (NS_FAILED(EncryptDataUCS2(passValue, entry->passValue)))
                  return NS_OK;

                writePasswords = PR_TRUE;
              }

              if (NS_SUCCEEDED(GetActionRealm(formElement, formActionOrigin)) &&
                  !entry->actionOrigin.Equals(formActionOrigin)) {
                
                entry->actionOrigin.Assign(formActionOrigin);

                writePasswords = PR_TRUE;
              }

              if (writePasswords)
                WritePasswords(mSignonFile);

              return NS_OK;
            }
          }
        }
      }

      nsresult rv;
      nsCOMPtr<nsIStringBundleService> bundleService =
        do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
      nsCOMPtr<nsIStringBundle> brandBundle;
      rv = bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                       getter_AddRefs(brandBundle));
      NS_ENSURE_SUCCESS(rv, rv);
      nsXPIDLString brandShortName;
      rv = brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                          getter_Copies(brandShortName));
      NS_ENSURE_SUCCESS(rv, rv);
      const PRUnichar* formatArgs[1] = { brandShortName.get() };

      nsAutoString dialogText;
      GetLocalizedString(NS_LITERAL_STRING("savePasswordText"),
                         dialogText,
                         PR_TRUE,
                         formatArgs,
                         1);

      nsAutoString dialogTitle, neverButtonText, rememberButtonText,
                   notNowButtonText;
      GetLocalizedString(NS_LITERAL_STRING("savePasswordTitle"), dialogTitle);

      GetLocalizedString(NS_LITERAL_STRING("neverForSiteButtonText"),
                         neverButtonText);
      GetLocalizedString(NS_LITERAL_STRING("rememberButtonText"),
                         rememberButtonText);
      GetLocalizedString(NS_LITERAL_STRING("notNowButtonText"),
                         notNowButtonText);

      PRInt32 selection;
      prompt->ConfirmEx(dialogTitle.get(),
                        dialogText.get(),
                        nsIPrompt::BUTTON_POS_1_DEFAULT +
                        (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
                        (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_1) +
                        (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_2),
                        rememberButtonText.get(),
                        notNowButtonText.get(),
                        neverButtonText.get(),
                        nsnull, nsnull,
                        &selection);

      if (selection == 0) {
        SignonDataEntry* entry = new SignonDataEntry();
        entry->userField.Assign(userFieldName);
        entry->passField.Assign(passFieldName);

        
        if (NS_FAILED(GetActionRealm(formElement, formActionOrigin))) {
          delete entry;
          return NS_OK;
        }

        entry->actionOrigin.Assign(formActionOrigin);

        if (NS_FAILED(EncryptDataUCS2(userValue, entry->userValue)) ||
            NS_FAILED(EncryptDataUCS2(passValue, entry->passValue))) {
          delete entry;
          return NS_OK;
        }

        AddSignonData(realm, entry);
        WritePasswords(mSignonFile);
      } else if (selection == 2) {
        AddReject(realm);
      }
    }
    break;

  case 2:
  case 3:
    {
      
      
      
      
      
      
      
      

      SignonDataEntry* changeEntry = nsnull;
      nsAutoString value0, valueN;
      passFields.ObjectAt(0)->GetValue(value0);

      for (PRInt32 k = 1; k < passFields.Count(); ++k) {
        passFields.ObjectAt(k)->GetValue(valueN);
        if (!value0.Equals(valueN)) {

          SignonHashEntry* hashEnt;

          if (mSignonTable.Get(realm, &hashEnt)) {

            SignonDataEntry* entry = hashEnt->head;

            if (entry->next) {

              
              

              PRUint32 entryCount = 2;
              SignonDataEntry* temp = entry->next;
              while (temp->next) {
                ++entryCount;
                temp = temp->next;
              }

              nsAutoString* ptUsernames = new nsAutoString[entryCount];
              const PRUnichar** formatArgs = new const PRUnichar*[entryCount];
              temp = entry;

              for (PRUint32 arg = 0; arg < entryCount; ++arg) {
                if (NS_FAILED(DecryptData(temp->userValue, ptUsernames[arg]))) {
                  delete [] formatArgs;
                  delete [] ptUsernames;
                  return NS_OK;
                }

                formatArgs[arg] = ptUsernames[arg].get();
                temp = temp->next;
              }

              nsAutoString dialogTitle, dialogText;
              GetLocalizedString(NS_LITERAL_STRING("passwordChangeTitle"),
                                 dialogTitle);
              GetLocalizedString(NS_LITERAL_STRING("userSelectText"),
                                 dialogText);

              PRInt32 selection;
              PRBool confirm;
              prompt->Select(dialogTitle.get(),
                             dialogText.get(),
                             entryCount,
                             formatArgs,
                             &selection,
                             &confirm);

              delete[] formatArgs;
              delete[] ptUsernames;

              if (confirm && selection >= 0) {
                changeEntry = entry;
                for (PRInt32 m = 0; m < selection; ++m)
                  changeEntry = changeEntry->next;
              }

            } else {
              nsAutoString dialogTitle, dialogText, ptUser;

              if (NS_FAILED(DecryptData(entry->userValue, ptUser)))
                return NS_OK;

              const PRUnichar* formatArgs[1] = { ptUser.get() };

              GetLocalizedString(NS_LITERAL_STRING("passwordChangeTitle"),
                                 dialogTitle);
              GetLocalizedString(NS_LITERAL_STRING("passwordChangeText"),
                                 dialogText,
                                 PR_TRUE,
                                 formatArgs,
                                 1);

              PRInt32 selection;
              prompt->ConfirmEx(dialogTitle.get(),
                                dialogText.get(),
                                nsIPrompt::STD_YES_NO_BUTTONS,
                                nsnull, nsnull, nsnull, nsnull, nsnull,
                                &selection);

              if (selection == 0)
                changeEntry = entry;
            }
          }
          break;
        }
      }

      if (changeEntry) {
        nsAutoString newValue;
        passFields.ObjectAt(1)->GetValue(newValue);
        if (NS_FAILED(EncryptDataUCS2(newValue, changeEntry->passValue)))
          return NS_OK;

        WritePasswords(mSignonFile);
      }
    }
    break;

  default:  
    break;
  }


  return NS_OK;
}



NS_IMETHODIMP
nsPasswordManager::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPasswordManager::Blur(nsIDOMEvent* aEvent)
{
  return FillPassword(aEvent);
}

NS_IMETHODIMP
nsPasswordManager::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  if (type.EqualsLiteral("DOMAutoComplete"))
    return FillPassword(aEvent);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(target);
  if (!domDoc)
    return NS_OK;

  if (type.EqualsLiteral("pagehide"))
    mAutoCompleteInputs.Enumerate(RemoveForDOMDocumentEnumerator, domDoc);
  else if (type.EqualsLiteral("DOMContentLoaded"))
    return FillDocument(domDoc);

  return NS_OK;
}




NS_IMETHODIMP
nsPasswordManager::GetPrompt(nsIDOMWindow* aParent, const nsIID& aIID,
                             void** _retval)
{
  if (!aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    NS_WARNING("asked for unknown IID");
    return NS_NOINTERFACE;
  }

  
  
  nsresult rv;
  nsCOMPtr<nsIPromptService2> service =
    do_GetService(NS_PROMPTSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsSingleSignonPrompt2* wrapper = new nsSingleSignonPrompt2(service, aParent);
  if (!wrapper)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(wrapper);
  *_retval = NS_STATIC_CAST(nsIAuthPrompt2*, wrapper);
  return NS_OK;
}



class UserAutoComplete : public nsIAutoCompleteResult
{
public:
  UserAutoComplete(const nsACString& aHost, const nsAString& aSearchString);
  virtual ~UserAutoComplete();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULT

  nsVoidArray mArray;
  nsCString   mHost;
  nsString    mSearchString;
  PRInt32     mDefaultIndex;
  PRUint16    mResult;
};

UserAutoComplete::UserAutoComplete(const nsACString& aHost,
                                   const nsAString& aSearchString)
  : mHost(aHost),
    mSearchString(aSearchString),
    mDefaultIndex(-1),
    mResult(RESULT_FAILURE)
{
}

UserAutoComplete::~UserAutoComplete()
{
  for (PRInt32 i  = 0; i < mArray.Count(); ++i)
    nsMemory::Free(mArray.ElementAt(i));
}

NS_IMPL_ISUPPORTS1(UserAutoComplete, nsIAutoCompleteResult)

NS_IMETHODIMP
UserAutoComplete::GetSearchString(nsAString& aString)
{
  aString.Assign(mSearchString);
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetSearchResult(PRUint16* aResult)
{
  *aResult = mResult;
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetDefaultIndex(PRInt32* aDefaultIndex)
{
  *aDefaultIndex = mDefaultIndex;
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetErrorDescription(nsAString& aDescription)
{
  aDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetMatchCount(PRUint32* aCount)
{
  *aCount = mArray.Count();
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetValueAt(PRInt32 aIndex, nsAString& aValue)
{
  aValue.Assign(NS_STATIC_CAST(PRUnichar*, mArray.ElementAt(aIndex)));
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetCommentAt(PRInt32 aIndex, nsAString& aComment)
{
  aComment.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::GetStyleAt(PRInt32 aIndex, nsAString& aHint)
{
  aHint.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
UserAutoComplete::RemoveValueAt(PRInt32 aIndex, PRBool aRemoveFromDB)
{
  NS_ENSURE_TRUE(aIndex >= 0 && aIndex < mArray.Count(), NS_ERROR_INVALID_ARG);

  PRUnichar *user = NS_STATIC_CAST(PRUnichar*, mArray.ElementAt(aIndex));
  if (aRemoveFromDB)
    sPasswordManager->RemoveUser(mHost, nsDependentString(user));

  nsMemory::Free(user);
  mArray.RemoveElementAt(aIndex);
  return NS_OK;
}

PR_STATIC_CALLBACK(int)
SortPRUnicharComparator(const void* aElement1,
                        const void* aElement2,
                        void* aData)
{
  return nsCRT::strcmp(NS_STATIC_CAST(const PRUnichar*, aElement1),
                       NS_STATIC_CAST(const PRUnichar*, aElement2));
}

PRBool
nsPasswordManager::AutoCompleteSearch(const nsAString& aSearchString,
                                      nsIAutoCompleteResult* aPreviousResult,
                                      nsIDOMHTMLInputElement* aElement,
                                      nsIAutoCompleteResult** aResult)
{
  PRInt32 dummy;
  if (!SingleSignonEnabled() || !mAutoCompleteInputs.Get(aElement, &dummy))
    return PR_FALSE;

  UserAutoComplete* result = nsnull;

  if (aPreviousResult) {

    
    

    result = NS_STATIC_CAST(UserAutoComplete*, aPreviousResult);

    if (result->mArray.Count()) {
      for (PRInt32 i = result->mArray.Count() - 1; i >= 0; --i) {
        nsDependentString match(NS_STATIC_CAST(PRUnichar*, result->mArray.ElementAt(i)));
        if (aSearchString.Length() > match.Length() ||
            !StringBeginsWith(match, aSearchString, nsCaseInsensitiveStringComparator())) {
          nsMemory::Free(result->mArray.ElementAt(i));
          result->mArray.RemoveElementAt(i);
        }
      }
    }
  } else {

    nsCOMPtr<nsIDOMDocument> domDoc;
    aElement->GetOwnerDocument(getter_AddRefs(domDoc));

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

    nsCAutoString realm;
    if (!GetPasswordRealm(doc->GetDocumentURI(), realm)) {
      *aResult = nsnull;
      return NS_OK;
    }

    

    result = new UserAutoComplete(realm, aSearchString);

    SignonHashEntry* hashEnt;
    if (mSignonTable.Get(realm, &hashEnt)) {
      
      
      
      

      mAutoCompletingField = aElement;

      nsCOMPtr<nsIDOMHTMLFormElement> formEl;
      aElement->GetForm(getter_AddRefs(formEl));
      if (!formEl)
        return NS_OK;
 
      nsCOMPtr<nsIForm> form = do_QueryInterface(formEl);
      nsCAutoString formActionOrigin;
 
      if (NS_FAILED(GetActionRealm(form, formActionOrigin)))
        return NS_OK;

      for (SignonDataEntry* e = hashEnt->head; e; e = e->next) {

        nsAutoString userValue;
        if (NS_FAILED(DecryptData(e->userValue, userValue)))
          return NS_ERROR_FAILURE;

        
        if (!e->actionOrigin.IsEmpty() &&
            !e->actionOrigin.Equals(formActionOrigin))
          continue;

        if (aSearchString.Length() <= userValue.Length() &&
            StringBeginsWith(userValue, aSearchString, nsCaseInsensitiveStringComparator())) {
          PRUnichar* data = ToNewUnicode(userValue);
          if (data)
            result->mArray.AppendElement(data);
        }
      }

      mAutoCompletingField = nsnull;
    }

    if (result->mArray.Count()) {
      result->mArray.Sort(SortPRUnicharComparator, nsnull);
      result->mResult = nsIAutoCompleteResult::RESULT_SUCCESS;
      result->mDefaultIndex = 0;
    } else {
      result->mResult = nsIAutoCompleteResult::RESULT_NOMATCH;
      result->mDefaultIndex = -1;
    }
  }

  *aResult = result;
  NS_ADDREF(*aResult);

  return PR_TRUE;
}

  PLDHashOperator PR_CALLBACK
nsPasswordManager::RemoveForDOMDocumentEnumerator(nsISupports* aKey,
                                                  PRInt32& aEntry,
                                                  void* aUserData)
{
  nsIDOMDocument* domDoc = NS_STATIC_CAST(nsIDOMDocument*, aUserData);
  nsCOMPtr<nsIDOMHTMLInputElement> element = do_QueryInterface(aKey);
  nsCOMPtr<nsIDOMDocument> elementDoc;
  element->GetOwnerDocument(getter_AddRefs(elementDoc));
  if (elementDoc == domDoc)
    return PL_DHASH_REMOVE;

  return PL_DHASH_NEXT;
}



























 PLDHashOperator PR_CALLBACK
nsPasswordManager::WriteRejectEntryEnumerator(const nsACString& aKey,
                                              PRInt32 aEntry,
                                              void* aUserData)
{
  nsIOutputStream* stream = NS_STATIC_CAST(nsIOutputStream*, aUserData);
  PRUint32 bytesWritten;

  nsCAutoString buffer(aKey);
  buffer.Append(NS_LINEBREAK);
  stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

  return PL_DHASH_NEXT;
}

 PLDHashOperator PR_CALLBACK
nsPasswordManager::WriteSignonEntryEnumerator(const nsACString& aKey,
                                              SignonHashEntry* aEntry,
                                              void* aUserData)
{
  nsIOutputStream* stream = NS_STATIC_CAST(nsIOutputStream*, aUserData);
  PRUint32 bytesWritten;

  nsCAutoString buffer(aKey);
  buffer.Append(NS_LINEBREAK);
  stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

  for (SignonDataEntry* e = aEntry->head; e; e = e->next) {
    NS_ConvertUTF16toUTF8 userField(e->userField);
    userField.Append(NS_LINEBREAK);
    stream->Write(userField.get(), userField.Length(), &bytesWritten);

    buffer.Assign(NS_ConvertUTF16toUTF8(e->userValue));
    buffer.Append(NS_LINEBREAK);
    stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

    buffer.Assign("*");
    buffer.Append(NS_ConvertUTF16toUTF8(e->passField));
    buffer.Append(NS_LINEBREAK);
    stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

    buffer.Assign(NS_ConvertUTF16toUTF8(e->passValue));
    buffer.Append(NS_LINEBREAK);
    stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

    buffer.Assign(e->actionOrigin);
    buffer.Append(NS_LINEBREAK);
    stream->Write(buffer.get(), buffer.Length(), &bytesWritten);
  }

  buffer.Assign("." NS_LINEBREAK);
  stream->Write(buffer.get(), buffer.Length(), &bytesWritten);

  return PL_DHASH_NEXT;
}


void
nsPasswordManager::LoadPasswords()
{
  if (sPasswordsLoaded)
    return;

  nsXPIDLCString signonFile;
  nsresult rv;
  rv = mPrefBranch->GetCharPref("SignonFileName2", getter_Copies(signonFile));
  if (NS_FAILED(rv))
    signonFile.Assign(NS_LITERAL_CSTRING("signons2.txt"));

  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mSignonFile));
  if (!mSignonFile)
    return;

  mSignonFile->AppendNative(signonFile);

  nsCAutoString path;
  mSignonFile->GetNativePath(path);

  PRBool signonExists = PR_FALSE;
  mSignonFile->Exists(&signonExists);
  if (signonExists) {
    if (NS_SUCCEEDED(ReadPasswords(mSignonFile)))
      sPasswordsLoaded = PR_TRUE;
  } else {
    
    rv = mPrefBranch->GetCharPref("SignonFileName", getter_Copies(signonFile));
    if (NS_FAILED(rv))
      signonFile.Assign(NS_LITERAL_CSTRING("signons.txt"));

    nsCOMPtr<nsIFile> oldSignonFile;
    mSignonFile->GetParent(getter_AddRefs(oldSignonFile));
    oldSignonFile->AppendNative(signonFile);

    if (NS_SUCCEEDED(ReadPasswords(oldSignonFile))) {
      sPasswordsLoaded = PR_TRUE;
      oldSignonFile->Remove(PR_FALSE);
    }
  }
}

void
nsPasswordManager::WritePasswords(nsIFile* aPasswordFile)
{
  nsCOMPtr<nsIOutputStream> fileStream;
  NS_NewLocalFileOutputStream(getter_AddRefs(fileStream), aPasswordFile, -1,
                              0600, 0);

  if (!fileStream)
    return;

  PRUint32 bytesWritten;

  
  nsCAutoString buffer("#2d" NS_LINEBREAK);
  fileStream->Write(buffer.get(), buffer.Length(), &bytesWritten);

  
  mRejectTable.EnumerateRead(WriteRejectEntryEnumerator, fileStream);

  buffer.Assign("." NS_LINEBREAK);
  fileStream->Write(buffer.get(), buffer.Length(), &bytesWritten);

  
  mSignonTable.EnumerateRead(WriteSignonEntryEnumerator, fileStream);
}

void
nsPasswordManager::AddSignonData(const nsACString& aRealm,
                                 SignonDataEntry* aEntry)
{
  
  SignonHashEntry* hashEnt;
  if (mSignonTable.Get(aRealm, &hashEnt)) {
    
    aEntry->next = hashEnt->head;
    hashEnt->head = aEntry;
  } else {
    mSignonTable.Put(aRealm, new SignonHashEntry(aEntry));
  }
}

 nsresult
nsPasswordManager::DecryptData(const nsAString& aData,
                               nsAString& aPlaintext)
{
  NS_ConvertUTF16toUTF8 flatData(aData);
  char* buffer = nsnull;

  if (flatData.CharAt(0) == '~') {

    
    PRUint32 srcLength = flatData.Length() - 1;

    if (!(buffer = PL_Base64Decode(&(flatData.get())[1], srcLength, NULL)))
      return NS_ERROR_FAILURE;

  } else {

    
    EnsureDecoderRing();
    if (!sDecoderRing) {
      NS_WARNING("Unable to get decoder ring service");
      return NS_ERROR_FAILURE;
    }

    if (NS_FAILED(sDecoderRing->DecryptString(flatData.get(), &buffer)))
      return NS_ERROR_FAILURE;

  }

  aPlaintext.Assign(NS_ConvertUTF8toUTF16(buffer));
  PR_Free(buffer);

  return NS_OK;
}






 nsresult
nsPasswordManager::EncryptData(const nsAString& aPlaintext,
                               nsACString& aEncrypted)
{
  EnsureDecoderRing();
  NS_ENSURE_TRUE(sDecoderRing, NS_ERROR_FAILURE);

  char* buffer;
  if (NS_FAILED(sDecoderRing->EncryptString(NS_ConvertUTF16toUTF8(aPlaintext).get(), &buffer)))
    return NS_ERROR_FAILURE;

  aEncrypted.Assign(buffer);
  PR_Free(buffer);

  return NS_OK;
}

 nsresult
nsPasswordManager::EncryptDataUCS2(const nsAString& aPlaintext,
                                   nsAString& aEncrypted)
{
  nsCAutoString buffer;
  nsresult rv = EncryptData(aPlaintext, buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  aEncrypted.Assign(NS_ConvertUTF8toUTF16(buffer));
  return NS_OK;
}

 void
nsPasswordManager::EnsureDecoderRing()
{
  if (!sDecoderRing) {
    CallGetService("@mozilla.org/security/sdr;1", &sDecoderRing);

    
    

    nsCOMPtr<nsIPK11TokenDB> tokenDB = do_GetService(NS_PK11TOKENDB_CONTRACTID);
    if (!tokenDB)
      return;

    nsCOMPtr<nsIPK11Token> token;
    tokenDB->GetInternalKeyToken(getter_AddRefs(token));

    PRBool needUserInit = PR_FALSE;
    token->GetNeedsUserInit(&needUserInit);

    if (needUserInit)
      token->InitPassword(EmptyString().get());
  }
}

nsresult
nsPasswordManager::FindPasswordEntryInternal(const SignonDataEntry* aEntry,
                                             const nsAString&  aUser,
                                             const nsAString&  aPassword,
                                             const nsAString&  aUserField,
                                             SignonDataEntry** aResult)
{
  
  const SignonDataEntry* entry = aEntry;
  nsAutoString buffer;

  for (; entry; entry = entry->next) {

    PRBool matched;

    if (aUser.IsEmpty()) {
      matched = PR_TRUE;
    } else {
      if (NS_FAILED(DecryptData(entry->userValue, buffer))) {
        *aResult = nsnull;
        return NS_ERROR_FAILURE;
      }
      matched = aUser.Equals(buffer);
    }

    if (!matched)
      continue;

    if (aPassword.IsEmpty()) {
      matched = PR_TRUE;
    } else {
      if (NS_FAILED(DecryptData(entry->passValue, buffer))) {
        *aResult = nsnull;
        return NS_ERROR_FAILURE;
      }
      matched = aPassword.Equals(buffer);
    }

    if (!matched)
      continue;

    if (aUserField.IsEmpty())
      matched = PR_TRUE;
    else
      matched = entry->userField.Equals(aUserField);

    if (matched)
      break;
  }

  if (entry) {
    *aResult = NS_CONST_CAST(SignonDataEntry*, entry);
    return NS_OK;
  }

  *aResult = nsnull;
  return NS_ERROR_FAILURE;
}

nsresult
nsPasswordManager::FillDocument(nsIDOMDocument* aDomDoc)
{
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(aDomDoc);
  if (!htmlDoc)
    return NS_OK;
  nsCOMPtr<nsIDOMHTMLCollection> forms;
  htmlDoc->GetForms(getter_AddRefs(forms));
  
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDomDoc);

  nsCAutoString realm;
  if (!GetPasswordRealm(doc->GetDocumentURI(), realm))
    return NS_OK;

  SignonHashEntry* hashEnt;
  if (!mSignonTable.Get(realm, &hashEnt))
    return NS_OK;

  PRUint32 formCount;
  forms->GetLength(&formCount);
  
  
  PRBool prefillForm = PR_TRUE;
  mPrefBranch->GetBoolPref("autofillForms", &prefillForm);

  nsCAutoString formActionOrigin;

  
  
  
  

  for (PRUint32 i = 0; i < formCount; ++i) {
    nsCOMPtr<nsIDOMNode> formNode;
    forms->Item(i, getter_AddRefs(formNode));

    nsCOMPtr<nsIForm> form = do_QueryInterface(formNode);
    SignonDataEntry* firstMatch = nsnull;
    PRBool attachedToInput = PR_FALSE;
    PRBool prefilledUser = PR_FALSE;
    nsCOMPtr<nsIDOMHTMLInputElement> userField, passField;
    nsCOMPtr<nsIDOMHTMLInputElement> temp;
    nsAutoString fieldType;

    
    if (NS_FAILED(GetActionRealm(form, formActionOrigin)))
      return NS_OK;

    for (SignonDataEntry* e = hashEnt->head; e; e = e->next) {

      nsCOMPtr<nsISupports> foundNode;
      if (!(e->userField).IsEmpty()) {
        form->ResolveName(e->userField, getter_AddRefs(foundNode));
        temp = do_QueryInterface(foundNode);
      }

      nsAutoString oldUserValue;
      PRBool userFieldFound = PR_FALSE;

      if (temp) {
        temp->GetType(fieldType);
        if (!fieldType.Equals(NS_LITERAL_STRING("text")))
          continue;

        temp->GetValue(oldUserValue);
        userField = temp;
        userFieldFound = PR_TRUE;
      } else if ((e->passField).IsEmpty()) {
        
        
        
        
        PRUint32 count;
        form->GetElementCount(&count);
        PRUint32 i;
        nsCOMPtr<nsIFormControl> formControl;
        for (i = 0; i < count; i++) {
          form->GetElementAt(i, getter_AddRefs(formControl));

          if (formControl &&
              formControl->GetType() == NS_FORM_INPUT_TEXT) {
            nsCOMPtr<nsIDOMHTMLInputElement> inputField = do_QueryInterface(formControl);
            nsAutoString name;
            inputField->GetName(name);
            if (name.EqualsIgnoreCase(NS_ConvertUTF16toUTF8(e->userField).get())) {
              inputField->GetValue(oldUserValue);
              userField = inputField;
              foundNode = inputField;
              e->userField.Assign(name);
              userFieldFound = PR_TRUE;
              break;
            }
          }
        }
      }

      
      if (!userFieldFound && !(e->userField).IsEmpty())
        continue;

      if (!(e->passField).IsEmpty()) {
        form->ResolveName(e->passField, getter_AddRefs(foundNode));
        temp = do_QueryInterface(foundNode);
      }
      else if (userField) {
        
        
        nsCOMPtr<nsIFormControl> fc(do_QueryInterface(foundNode));
        PRInt32 index = -1;
        form->IndexOfControl(fc, &index);
        if (index >= 0) {
          PRUint32 count;
          form->GetElementCount(&count);

          PRUint32 i;
          temp = nsnull;

          
          nsCOMPtr<nsIFormControl> passField;
          for (i = index + 1; i < count; ++i) {
            form->GetElementAt(i, getter_AddRefs(passField));

            if (passField && passField->GetType() == NS_FORM_INPUT_PASSWORD) {
              foundNode = passField;
              temp = do_QueryInterface(foundNode);
            }
          }

          if (!temp && index != 0) {
            
            i = index;
            do {
              form->GetElementAt(i, getter_AddRefs(passField));

              if (passField && passField->GetType() == NS_FORM_INPUT_PASSWORD) {
                foundNode = passField;
                temp = do_QueryInterface(foundNode);
              }
            } while (i-- != 0);
          }
        }
      }

      nsAutoString oldPassValue;

      if (temp) {
        temp->GetType(fieldType);
        if (!fieldType.Equals(NS_LITERAL_STRING("password")))
          continue;

        temp->GetValue(oldPassValue);
        passField = temp;
        if ((e->passField).IsEmpty())
          passField->GetName(e->passField);
      } else {
        continue;
      }

      
      if (!e->actionOrigin.IsEmpty() &&
          !e->actionOrigin.Equals(formActionOrigin))
        continue;

      if (!oldUserValue.IsEmpty() && prefillForm) {
        
        
        
        

        prefilledUser = PR_TRUE;
        nsAutoString userValue;
        if (NS_FAILED(DecryptData(e->userValue, userValue)))
          return NS_OK;

        if (userValue.Equals(oldUserValue)) {
          nsAutoString passValue;
          if (NS_FAILED(DecryptData(e->passValue, passValue)))
            return NS_OK;

          passField->SetValue(passValue);
        }
      }

      if (firstMatch && userField && !attachedToInput) {
        

        
        
        

        AttachToInput(userField);
        attachedToInput = PR_TRUE;
      } else {
        firstMatch = e;
      }
    }

    
    
    
    
    if (firstMatch && !attachedToInput) {
      if (!prefilledUser && prefillForm) {
        nsAutoString buffer;

        if (userField) {
          if (NS_FAILED(DecryptData(firstMatch->userValue, buffer)))
            return NS_OK;

          userField->SetValue(buffer);
        }

        if (NS_FAILED(DecryptData(firstMatch->passValue, buffer)))
          return NS_OK;

        passField->SetValue(buffer);
      }

      if (userField)
        AttachToInput(userField);
    }
  }

  return NS_OK;
}

nsresult
nsPasswordManager::FillPassword(nsIDOMEvent* aEvent)
{
  
  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  nsCOMPtr<nsIDOMHTMLInputElement> userField = do_QueryInterface(target);
  if (!userField || userField == mAutoCompletingField)
    return NS_OK;

  nsCOMPtr<nsIContent> fieldContent = do_QueryInterface(userField);

  
  
  nsIDocument *doc = fieldContent->GetDocument();
  if (!doc)
    return NS_OK;

  nsCAutoString realm;
  if (!GetPasswordRealm(doc->GetDocumentURI(), realm))
    return NS_OK;

  nsAutoString userValue;
  userField->GetValue(userValue);

  if (userValue.IsEmpty())
    return NS_OK;

  nsAutoString fieldName;
  userField->GetName(fieldName);

  SignonHashEntry* hashEnt;
  if (!mSignonTable.Get(realm, &hashEnt))
    return NS_OK;

  SignonDataEntry* foundEntry;
  FindPasswordEntryInternal(hashEnt->head, userValue, EmptyString(),
                            fieldName, &foundEntry);

  if (!foundEntry)
    return NS_OK;

  nsCOMPtr<nsIDOMHTMLFormElement> formEl;
  userField->GetForm(getter_AddRefs(formEl));
  if (!formEl)
    return NS_OK;

  nsCOMPtr<nsIForm> form = do_QueryInterface(formEl);
  nsCAutoString formActionOrigin;
  GetActionRealm(form, formActionOrigin);
  if (NS_FAILED(GetActionRealm(form, formActionOrigin)))
    return NS_OK;
  if (!foundEntry->actionOrigin.IsEmpty() && !foundEntry->actionOrigin.Equals(formActionOrigin))
    return NS_OK;

  nsCOMPtr<nsISupports> foundNode;
  form->ResolveName(foundEntry->passField, getter_AddRefs(foundNode));
  nsCOMPtr<nsIDOMHTMLInputElement> passField = do_QueryInterface(foundNode);
  if (!passField)
    return NS_OK;

  nsAutoString passValue;
  if (NS_SUCCEEDED(DecryptData(foundEntry->passValue, passValue)))
    passField->SetValue(passValue);

  return NS_OK;
}

void
nsPasswordManager::AttachToInput(nsIDOMHTMLInputElement* aElement)
{
  nsCOMPtr<nsIDOMEventTarget> targ = do_QueryInterface(aElement);
  nsIDOMEventListener* listener = NS_STATIC_CAST(nsIDOMFocusListener*, this);

  targ->AddEventListener(NS_LITERAL_STRING("blur"), listener, PR_FALSE);
  targ->AddEventListener(NS_LITERAL_STRING("DOMAutoComplete"), listener, PR_FALSE);

  mAutoCompleteInputs.Put(aElement, 1);
}

PRBool
nsPasswordManager::GetPasswordRealm(nsIURI* aURI, nsACString& aRealm)
{
  
  
  
  

  nsCAutoString buffer;
  aURI->GetScheme(buffer);

  aRealm.Append(buffer);
  aRealm.Append(NS_LITERAL_CSTRING("://"));

  aURI->GetHostPort(buffer);
  if (buffer.IsEmpty()) {
    
    
    return PR_FALSE;
  }

  aRealm.Append(buffer);
  return PR_TRUE;
}

 void
nsPasswordManager::GetLocalizedString(const nsAString& key,
                                      nsAString& aResult,
                                      PRBool aIsFormatted,
                                      const PRUnichar** aFormatArgs,
                                      PRUint32 aFormatArgsLength)
{
  if (!sPMBundle) {
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    bundleService->CreateBundle(kPMPropertiesURL,
                                &sPMBundle);

    if (!sPMBundle) {
      NS_ERROR("string bundle not present");
      return;
    }
  }

  nsXPIDLString str;
  if (aIsFormatted)
    sPMBundle->FormatStringFromName(PromiseFlatString(key).get(),
                                    aFormatArgs, aFormatArgsLength,
                                    getter_Copies(str));
  else
    sPMBundle->GetStringFromName(PromiseFlatString(key).get(),
                                 getter_Copies(str));
  aResult.Assign(str);
}

 nsresult
nsPasswordManager::GetActionRealm(nsIForm* aForm, nsCString& aURL)
{
  nsCOMPtr<nsIURI> actionURI;
  nsCAutoString formActionOrigin;

  if (NS_FAILED(aForm->GetActionURL(getter_AddRefs(actionURI))) ||
      !actionURI)
    return NS_ERROR_FAILURE;

  if (!GetPasswordRealm(actionURI, formActionOrigin))
    return NS_ERROR_FAILURE;

  aURL.Assign(formActionOrigin);
  return NS_OK;
}
