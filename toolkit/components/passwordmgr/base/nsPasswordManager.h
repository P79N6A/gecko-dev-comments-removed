





































#include "nsCPasswordManager.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIFormSubmitObserver.h"
#include "nsIWebProgressListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIStringBundle.h"
#include "nsIPrefBranch.h"
#include "nsIPromptFactory.h"


#define NS_PASSWORDMANAGER_CID \
{0x360565c4, 0x2ef3, 0x4f6a, {0xba, 0xb9, 0x94, 0xcc, 0xa8, 0x91, 0xb2, 0xa7}}

class nsIFile;
class nsIStringBundle;
class nsIComponentManager;
class nsIDOMWindowInternal;
class nsIForm;
class nsIURI;
class nsIDOMHTMLInputElement;
class nsIAutoCompleteResult;
struct nsModuleComponentInfo;

class nsPasswordManager : public nsIPasswordManager,
                          public nsIPasswordManagerInternal,
                          public nsIObserver,
                          public nsIFormSubmitObserver,
                          public nsIWebProgressListener,
                          public nsIDOMFocusListener,
                          public nsIPromptFactory,
                          public nsSupportsWeakReference
{
public:
  class SignonDataEntry;
  class SignonHashEntry;
  class PasswordEntry;

  nsPasswordManager();
  virtual ~nsPasswordManager();

  static nsPasswordManager* GetInstance();

  nsresult Init();
  static PRBool SingleSignonEnabled();

  static NS_METHOD Register(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* aRegistryLocation,
                            const char* aComponentType,
                            const nsModuleComponentInfo* aInfo);

  static NS_METHOD Unregister(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* aRegistryLocation,
                              const nsModuleComponentInfo* aInfo);

  static void Shutdown();

  static void GetLocalizedString(const nsAString& key,
                                 nsAString& aResult,
                                 PRBool aFormatted = PR_FALSE,
                                 const PRUnichar** aFormatArgs = nsnull,
                                 PRUint32 aFormatArgsLength = 0);

  static nsresult DecryptData(const nsAString& aData, nsAString& aPlaintext);
  static nsresult EncryptData(const nsAString& aPlaintext,
                              nsACString& aEncrypted);
  static nsresult EncryptDataUCS2(const nsAString& aPlaintext,
                                  nsAString& aEncrypted);


  NS_DECL_ISUPPORTS
  NS_DECL_NSIPASSWORDMANAGER
  NS_DECL_NSIPASSWORDMANAGERINTERNAL
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIPROMPTFACTORY

  
  NS_IMETHOD Notify(nsIDOMHTMLFormElement* aDOMForm,
                    nsIDOMWindowInternal* aWindow,
                    nsIURI* aActionURL,
                    PRBool* aCancelSubmit);

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  PRBool AutoCompleteSearch(const nsAString& aSearchString,
                            nsIAutoCompleteResult* aPreviousResult,
                            nsIDOMHTMLInputElement* aElement,
                            nsIAutoCompleteResult** aResult);

protected:
  void LoadPasswords();
  void WritePasswords(nsIFile* aPasswordFile);
  void AddSignonData(const nsACString& aRealm, SignonDataEntry* aEntry);

  nsresult FindPasswordEntryInternal(const SignonDataEntry* aEntry,
                                     const nsAString&  aUser,
                                     const nsAString&  aPassword,
                                     const nsAString&  aUserField,
                                     SignonDataEntry** aResult);

  nsresult FillDocument(nsIDOMDocument* aDomDoc);
  nsresult FillPassword(nsIDOMEvent* aEvent);
  void AttachToInput(nsIDOMHTMLInputElement* aElement);
  static PRBool GetPasswordRealm(nsIURI* aURI, nsACString& aRealm);
  
  static nsresult GetActionRealm(nsIForm* aForm, nsCString& aURL);

  static PLDHashOperator PR_CALLBACK FindEntryEnumerator(const nsACString& aKey,
                                                         SignonHashEntry* aEntry,
                                                         void* aUserData);

  static PLDHashOperator PR_CALLBACK WriteRejectEntryEnumerator(const nsACString& aKey,
                                                                PRInt32 aEntry,
                                                                void* aUserData);

  static PLDHashOperator PR_CALLBACK WriteSignonEntryEnumerator(const nsACString& aKey,
                                                                SignonHashEntry* aEntry,
                                                                void* aUserData);

  static PLDHashOperator PR_CALLBACK BuildArrayEnumerator(const nsACString& aKey,
                                                          SignonHashEntry* aEntry,
                                                          void* aUserData);

  static PLDHashOperator PR_CALLBACK BuildRejectArrayEnumerator(const nsACString& aKey,
                                                                PRInt32 aEntry,
                                                                void* aUserData);

  static PLDHashOperator PR_CALLBACK RemoveForDOMDocumentEnumerator(nsISupports* aKey,
                                                                    PRInt32& aEntry,
                                                                    void* aUserData);

  static void EnsureDecoderRing();

  nsClassHashtable<nsCStringHashKey,SignonHashEntry> mSignonTable;
  nsDataHashtable<nsCStringHashKey,PRInt32> mRejectTable;
  nsDataHashtable<nsISupportsHashKey,PRInt32> mAutoCompleteInputs;

  nsCOMPtr<nsIFile> mSignonFile;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsIDOMHTMLInputElement* mAutoCompletingField;
};
