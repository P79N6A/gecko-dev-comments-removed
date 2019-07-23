




































#ifndef _NS_CERTTREE_H_
#define _NS_CERTTREE_H_

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsICertTree.h"
#include "nsITreeView.h"
#include "nsITreeBoxObject.h"
#include "nsITreeSelection.h"
#include "nsISupportsArray.h"
#include "nsIMutableArray.h"
#include "pldhash.h"
#include "nsIX509CertDB.h"

typedef struct treeArrayElStr treeArrayEl;

struct CompareCacheHashEntry {
  enum { max_criterions = 3 };
  CompareCacheHashEntry();

  void *key; 
  PRPackedBool mCritInit[max_criterions];
  nsXPIDLString mCrit[max_criterions];
};

struct CompareCacheHashEntryPtr : PLDHashEntryHdr {
  CompareCacheHashEntryPtr();
  ~CompareCacheHashEntryPtr();
  CompareCacheHashEntry *entry;
};


class nsCertTree : public nsICertTree
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICERTTREE
  NS_DECL_NSITREEVIEW

  nsCertTree();
  virtual ~nsCertTree();

  enum sortCriterion { sort_IssuerOrg, sort_Org, sort_Token, 
    sort_CommonName, sort_IssuedDateDescending, sort_Email, sort_None };

protected:
  nsresult InitCompareHash();
  void ClearCompareHash();
  void RemoveCacheEntry(void *key);

  typedef int (*nsCertCompareFunc)(void *, nsIX509Cert *a, nsIX509Cert *b);

  static CompareCacheHashEntry *getCacheEntry(void *cache, void *aCert);
  static void CmpInitCriterion(nsIX509Cert *cert, CompareCacheHashEntry *entry,
                               sortCriterion crit, PRInt32 level);
  static PRInt32 CmpByCrit(nsIX509Cert *a, CompareCacheHashEntry *ace, 
                           nsIX509Cert *b, CompareCacheHashEntry *bce, 
                           sortCriterion crit, PRInt32 level);
  static PRInt32 CmpBy(void *cache, nsIX509Cert *a, nsIX509Cert *b, 
                       sortCriterion c0, sortCriterion c1, sortCriterion c2);
  static PRInt32 CmpCACert(void *cache, nsIX509Cert *a, nsIX509Cert *b);
  static PRInt32 CmpWebSiteCert(void *cache, nsIX509Cert *a, nsIX509Cert *b);
  static PRInt32 CmpUserCert(void *cache, nsIX509Cert *a, nsIX509Cert *b);
  static PRInt32 CmpEmailCert(void *cache, nsIX509Cert *a, nsIX509Cert *b);
  nsCertCompareFunc GetCompareFuncFromCertType(PRUint32 aType);
  PRInt32 CountOrganizations();

  PRBool GetCertsByType(PRUint32 aType, nsCertCompareFunc aCertCmpFn,
                        void *aCertCmpFnArg, nsISupportsArray **_certs);

  PRBool GetCertsByTypeFromCache(nsINSSCertCache *aCache, PRUint32 aType,
                                 nsCertCompareFunc aCertCmpFn, void *aCertCmpFnArg,
                                 nsISupportsArray **_certs);
private:
  nsCOMPtr<nsISupportsArray>      mCertArray;
  nsCOMPtr<nsITreeBoxObject>  mTree;
  nsCOMPtr<nsITreeSelection>  mSelection;
  treeArrayEl                *mTreeArray;
  PRInt32                         mNumOrgs;
  PRInt32                         mNumRows;
  PLDHashTable mCompareCache;
  nsCOMPtr<nsINSSComponent> mNSSComponent;

  treeArrayEl *GetThreadDescAtIndex(PRInt32 _index);
  nsIX509Cert *GetCertAtIndex(PRInt32 _index, PRInt32 *outAbsoluteCertOffset = nsnull);

  void FreeCertArray();
  nsresult UpdateUIContents();

  PRBool GetCertsByTypeFromCertList(CERTCertList *aCertList,
                                    PRUint32 aType,
                                    nsCertCompareFunc  aCertCmpFn,
                                    void              *aCertCmpFnArg,
                                    nsISupportsArray **_certs);

  nsCOMPtr<nsIMutableArray> mCellText;

#ifdef DEBUG_CERT_TREE
  
  void dumpMap();
#endif
};

#endif 

