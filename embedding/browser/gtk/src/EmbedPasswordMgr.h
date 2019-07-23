







































#include "nsCPasswordManager.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIFormSubmitObserver.h"
#include "nsIWebProgressListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIStringBundle.h"
#include "nsIPrefBranch.h"
#include "nsIPromptFactory.h"
#include "nsIAuthPromptWrapper.h"
#include "nsCOMPtr.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt2.h"
#include "EmbedPrivate.h"

#define EMBED_PASSWORDMANAGER_DESCRIPTION "MicroB PSM Dialog Impl"

#define EMBED_PASSWORDMANAGER_CID \
{0x360565c4, 0x2ef3, 0x4f6a, {0xba, 0xb9, 0x94, 0xcc, 0xa8, 0x91, 0xb2, 0xa7}}

class nsIFile;
class nsIStringBundle;
class nsIComponentManager;
class nsIContent;
class nsIDOMWindowInternal;
class nsIURI;
class nsIDOMHTMLInputElement;
class nsIDOMWindow;
class nsIPromptService2;

struct nsModuleComponentInfo;

class EmbedPasswordMgr : public nsIPasswordManager,
                         public nsIPasswordManagerInternal,
                         public nsIObserver,
                         public nsIFormSubmitObserver,
                         public nsIWebProgressListener,
                         public nsIDOMFocusListener,
                         public nsIPromptFactory,
                         public nsIDOMLoadListener,
                         public nsSupportsWeakReference
{
public:
  class SignonDataEntry;
  class SignonHashEntry;
  class PasswordEntry;
  EmbedPasswordMgr();
  virtual ~EmbedPasswordMgr();
  static EmbedPasswordMgr* GetInstance();
  static EmbedPasswordMgr* GetInstance(EmbedPrivate *aOwner);
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
  nsresult InsertLogin(const char* username, const char* password = nsnull);
  nsresult RemovePasswords(const char *aHostName, const char *aUserName);
  nsresult RemovePasswordsByIndex(PRUint32 aIndex);
  nsresult IsEqualToLastHostQuery(nsCString& aHost);
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
  
  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD Unload(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent);
  NS_IMETHOD Abort(nsIDOMEvent* aEvent);
  NS_IMETHOD Error(nsIDOMEvent* aEvent);
protected:
  void WritePasswords(nsIFile* aPasswordFile);
  void AddSignonData(const nsACString& aRealm, SignonDataEntry* aEntry);
  nsresult FindPasswordEntryInternal(const SignonDataEntry* aEntry,
                                     const nsAString&  aUser,
                                     const nsAString&  aPassword,
                                     const nsAString&  aUserField,
                                     SignonDataEntry** aResult);
  nsresult FillPassword(nsIDOMEvent* aEvent = nsnull);
  void AttachToInput(nsIDOMHTMLInputElement* aElement);
  PRBool GetPasswordRealm(nsIURI* aURI, nsACString& aRealm);
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
  nsIDOMHTMLInputElement* mGlobalUserField;
  nsIDOMHTMLInputElement* mGlobalPassField;
  SignonHashEntry * mLastSignonHashEntry;
  int lastIndex;
  nsCAutoString mLastHostQuery;
  EmbedCommon* mCommonObject;
public:
  PRBool mFormAttachCount;
  
};


#define NS_SINGLE_SIGNON_PROMPT_CID \
{0x1baf3398, 0xf759, 0x4a72, {0xa2, 0x1f, 0x0a, 0xbd, 0xc9, 0xcc, 0x99, 0x60}}



class EmbedSignonPrompt : public nsIAuthPromptWrapper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHPROMPT
  NS_DECL_NSIAUTHPROMPTWRAPPER
  EmbedSignonPrompt() {}
  virtual ~EmbedSignonPrompt() {}
protected:
  void GetLocalizedString(const nsAString& aKey, nsAString& aResult);
  nsCOMPtr<nsIPrompt> mPrompt;
};





class EmbedSignonPrompt2 : public nsIAuthPrompt2
{
public:
  EmbedSignonPrompt2(nsIPromptService2* aService, nsIDOMWindow* aParent);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHPROMPT2

private:
  ~EmbedSignonPrompt2();

  nsCOMPtr<nsIPromptService2> mService;
  nsCOMPtr<nsIDOMWindow> mParent;
};

