





































#include "nsNSSComponent.h" 
#include "nsCertTree.h"
#include "nsITreeColumns.h"
#include "nsIX509Cert.h"
#include "nsIX509CertValidity.h"
#include "nsIX509CertDB.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertHelper.h"
#include "nsINSSCertCache.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsXPCOMCID.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"


#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

#include "nsNSSCleaner.h"
NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);






struct treeArrayElStr {
  nsString   orgName;     
  PRBool     open;        
  PRInt32    certIndex;   
  PRInt32    numChildren; 
};

CompareCacheHashEntryPtr::CompareCacheHashEntryPtr()
{
  entry = new CompareCacheHashEntry;
}

CompareCacheHashEntryPtr::~CompareCacheHashEntryPtr()
{
  delete entry;
}

CompareCacheHashEntry::CompareCacheHashEntry()
:key(nsnull)
{
  for (int i = 0; i < max_criterions; ++i) {
    mCritInit[i] = PR_FALSE;
  }
}

PR_STATIC_CALLBACK(PRBool)
CompareCacheMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const CompareCacheHashEntryPtr *entryPtr = static_cast<const CompareCacheHashEntryPtr*>(hdr);
  return entryPtr->entry->key == key;
}

PR_STATIC_CALLBACK(PRBool)
CompareCacheInitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     const void *key)
{
  new (hdr) CompareCacheHashEntryPtr();
  CompareCacheHashEntryPtr *entryPtr = static_cast<CompareCacheHashEntryPtr*>(hdr);
  if (!entryPtr->entry) {
    return PR_FALSE;
  }
  entryPtr->entry->key = (void*)key;
  return PR_TRUE;
}

PR_STATIC_CALLBACK(void)
CompareCacheClearEntry(PLDHashTable *table, PLDHashEntryHdr *hdr)
{
  CompareCacheHashEntryPtr *entryPtr = static_cast<CompareCacheHashEntryPtr*>(hdr);
  entryPtr->~CompareCacheHashEntryPtr();
}

static PLDHashTableOps gMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  CompareCacheMatchEntry,
  PL_DHashMoveEntryStub,
  CompareCacheClearEntry,
  PL_DHashFinalizeStub,
  CompareCacheInitEntry
};

NS_IMPL_ISUPPORTS0(nsCertAddonInfo)
NS_IMPL_ISUPPORTS1(nsCertTreeDispInfo, nsICertTreeItem)

nsCertTreeDispInfo::nsCertTreeDispInfo()
:mAddonInfo(nsnull)
,mTypeOfEntry(direct_db)
,mPort(-1)
,mOverrideBits(nsCertOverride::ob_None)
,mIsTemporary(PR_TRUE)
{
}

nsCertTreeDispInfo::nsCertTreeDispInfo(nsCertTreeDispInfo &other)
{
  mAddonInfo = other.mAddonInfo;
  mTypeOfEntry = other.mTypeOfEntry;
  mAsciiHost = other.mAsciiHost;
  mPort = other.mPort;
  mOverrideBits = other.mOverrideBits;
  mIsTemporary = other.mIsTemporary;
  mCert = other.mCert;
}

nsCertTreeDispInfo::~nsCertTreeDispInfo()
{
}

NS_IMETHODIMP
nsCertTreeDispInfo::GetCert(nsIX509Cert **_cert)
{
  NS_ENSURE_ARG(_cert);
  if (mCert) {
    
    *_cert = mCert;
    NS_IF_ADDREF(*_cert);
    return NS_OK;
  }
  if (mAddonInfo) {
    *_cert = mAddonInfo->mCert.get();
    NS_IF_ADDREF(*_cert);
  }
  else {
    *_cert = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCertTreeDispInfo::GetHostPort(nsAString &aHostPort)
{
  nsCAutoString hostPort;
  nsCertOverrideService::GetHostWithPort(mAsciiHost, mPort, hostPort);
  aHostPort = NS_ConvertUTF8toUTF16(hostPort);
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsCertTree, nsICertTree, nsITreeView)

nsCertTree::nsCertTree() : mTreeArray(NULL)
{
  mCompareCache.ops = nsnull;
  mNSSComponent = do_GetService(kNSSComponentCID);
  mOverrideService = do_GetService("@mozilla.org/security/certoverride;1");
  mCellText = nsnull;
}

void nsCertTree::ClearCompareHash()
{
  if (mCompareCache.ops) {
    PL_DHashTableFinish(&mCompareCache);
    mCompareCache.ops = nsnull;
  }
}

nsresult nsCertTree::InitCompareHash()
{
  ClearCompareHash();
  if (!PL_DHashTableInit(&mCompareCache, &gMapOps, nsnull,
                         sizeof(CompareCacheHashEntryPtr), 128)) {
    mCompareCache.ops = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsCertTree::~nsCertTree()
{
  ClearCompareHash();
  delete [] mTreeArray;
}

void
nsCertTree::FreeCertArray()
{
  mDispInfo.Clear();
}

CompareCacheHashEntry *
nsCertTree::getCacheEntry(void *cache, void *aCert)
{
  PLDHashTable &aCompareCache = *reinterpret_cast<PLDHashTable*>(cache);
  CompareCacheHashEntryPtr *entryPtr = 
    static_cast<CompareCacheHashEntryPtr*>
               (PL_DHashTableOperate(&aCompareCache, aCert, PL_DHASH_ADD));
  return entryPtr ? entryPtr->entry : NULL;
}

void nsCertTree::RemoveCacheEntry(void *key)
{
  PL_DHashTableOperate(&mCompareCache, key, PL_DHASH_REMOVE);
}





PRInt32
nsCertTree::CountOrganizations()
{
  PRUint32 i, certCount;
  certCount = mDispInfo.Length();
  if (certCount == 0) return 0;
  nsCOMPtr<nsIX509Cert> orgCert = nsnull;
  if (mDispInfo.ElementAt(0)->mAddonInfo) {
    orgCert = mDispInfo.ElementAt(0)->mAddonInfo->mCert;
  }
  nsCOMPtr<nsIX509Cert> nextCert = nsnull;
  PRInt32 orgCount = 1;
  for (i=1; i<certCount; i++) {
    nextCert = nsnull;
    if (mDispInfo.ElementAt(i)->mAddonInfo) {
      nextCert = mDispInfo.ElementAt(i)->mAddonInfo->mCert;
    }
    
    if (CmpBy(&mCompareCache, orgCert, nextCert, sort_IssuerOrg, sort_None, sort_None) != 0) {
      orgCert = nextCert;
      orgCount++;
    }
  }
  return orgCount;
}





treeArrayEl *
nsCertTree::GetThreadDescAtIndex(PRInt32 index)
{
  int i, idx=0;
  if (index < 0) return nsnull;
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx) {
      return &mTreeArray[i];
    }
    if (mTreeArray[i].open) {
      idx += mTreeArray[i].numChildren;
    }
    idx++;
    if (idx > index) break;
  }
  return nsnull;
}




already_AddRefed<nsIX509Cert>
nsCertTree::GetCertAtIndex(PRInt32 index, PRInt32 *outAbsoluteCertOffset)
{
  nsRefPtr<nsCertTreeDispInfo> certdi =
    GetDispInfoAtIndex(index, outAbsoluteCertOffset);
  if (!certdi)
    return nsnull;

  nsIX509Cert *rawPtr = nsnull;
  if (certdi->mCert) {
    rawPtr = certdi->mCert;
  } else if (certdi->mAddonInfo) {
    rawPtr = certdi->mAddonInfo->mCert;
  }
  NS_IF_ADDREF(rawPtr);
  return rawPtr;
}


already_AddRefed<nsCertTreeDispInfo>
nsCertTree::GetDispInfoAtIndex(PRInt32 index, 
                               PRInt32 *outAbsoluteCertOffset)
{
  int i, idx = 0, cIndex = 0, nc;
  if (index < 0) return nsnull;
  
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx) return nsnull; 
    idx++; 
    nc = (mTreeArray[i].open) ? mTreeArray[i].numChildren : 0;
    if (index < idx + nc) { 
      PRInt32 certIndex = cIndex + index - idx;
      if (outAbsoluteCertOffset)
        *outAbsoluteCertOffset = certIndex;
      nsRefPtr<nsCertTreeDispInfo> certdi = mDispInfo.ElementAt(certIndex);
      if (certdi) {
        nsCertTreeDispInfo *raw = certdi.get();
        NS_IF_ADDREF(raw);
        return raw;
      }
      break;
    }
    if (mTreeArray[i].open)
      idx += mTreeArray[i].numChildren;
    cIndex += mTreeArray[i].numChildren;
    if (idx > index) break;
  }
  return nsnull;
}

nsCertTree::nsCertCompareFunc
nsCertTree::GetCompareFuncFromCertType(PRUint32 aType)
{
  switch (aType) {
    case nsIX509Cert2::ANY_CERT:
    case nsIX509Cert::USER_CERT:
      return CmpUserCert;
    case nsIX509Cert::CA_CERT:
      return CmpCACert;
    case nsIX509Cert::EMAIL_CERT:
      return CmpEmailCert;
    case nsIX509Cert::SERVER_CERT:
    default:
      return CmpWebSiteCert;
  }
}

struct nsCertAndArrayAndPositionAndCounterAndTracker
{
  nsRefPtr<nsCertAddonInfo> certai;
  nsTArray< nsRefPtr<nsCertTreeDispInfo> > *array;
  int position;
  int counter;
  nsTHashtable<nsCStringHashKey> *tracker;
};








PR_STATIC_CALLBACK(void)
MatchingCertOverridesCallback(const nsCertOverride &aSettings,
                              void *aUserData)
{
  nsCertAndArrayAndPositionAndCounterAndTracker *cap = 
    (nsCertAndArrayAndPositionAndCounterAndTracker*)aUserData;
  if (!cap)
    return;

  nsCertTreeDispInfo *certdi = new nsCertTreeDispInfo;
  if (certdi) {
    if (cap->certai)
      cap->certai->mUsageCount++;
    certdi->mAddonInfo = cap->certai;
    certdi->mTypeOfEntry = nsCertTreeDispInfo::host_port_override;
    certdi->mAsciiHost = aSettings.mAsciiHost;
    certdi->mPort = aSettings.mPort;
    certdi->mOverrideBits = aSettings.mOverrideBits;
    certdi->mIsTemporary = aSettings.mIsTemporary;
    certdi->mCert = aSettings.mCert;
    cap->array->InsertElementAt(cap->position, certdi);
    cap->position++;
    cap->counter++;
  }

  
  
  nsCAutoString hostPort;
  nsCertOverrideService::GetHostWithPort(aSettings.mAsciiHost, aSettings.mPort, hostPort);
  cap->tracker->RemoveEntry(hostPort);
}



PR_STATIC_CALLBACK(void)
CollectAllHostPortOverridesCallback(const nsCertOverride &aSettings,
                                    void *aUserData)
{
  nsTHashtable<nsCStringHashKey> *collectorTable =
    (nsTHashtable<nsCStringHashKey> *)aUserData;
  if (!collectorTable)
    return;

  nsCAutoString hostPort;
  nsCertOverrideService::GetHostWithPort(aSettings.mAsciiHost, aSettings.mPort, hostPort);
  collectorTable->PutEntry(hostPort);
}

struct nsArrayAndPositionAndCounterAndTracker
{
  nsTArray< nsRefPtr<nsCertTreeDispInfo> > *array;
  int position;
  int counter;
  nsTHashtable<nsCStringHashKey> *tracker;
};



PR_STATIC_CALLBACK(void)
AddRemaningHostPortOverridesCallback(const nsCertOverride &aSettings,
                                     void *aUserData)
{
  nsArrayAndPositionAndCounterAndTracker *cap = 
    (nsArrayAndPositionAndCounterAndTracker*)aUserData;
  if (!cap)
    return;

  nsCAutoString hostPort;
  nsCertOverrideService::GetHostWithPort(aSettings.mAsciiHost, aSettings.mPort, hostPort);
  if (!cap->tracker->GetEntry(hostPort))
    return;

  
  

  nsCertTreeDispInfo *certdi = new nsCertTreeDispInfo;
  if (certdi) {
    certdi->mAddonInfo = nsnull;
    certdi->mTypeOfEntry = nsCertTreeDispInfo::host_port_override;
    certdi->mAsciiHost = aSettings.mAsciiHost;
    certdi->mPort = aSettings.mPort;
    certdi->mOverrideBits = aSettings.mOverrideBits;
    certdi->mIsTemporary = aSettings.mIsTemporary;
    certdi->mCert = aSettings.mCert;
    cap->array->InsertElementAt(cap->position, certdi);
    cap->position++;
    cap->counter++;
  }
}

nsresult
nsCertTree::GetCertsByTypeFromCertList(CERTCertList *aCertList,
                                       PRUint32 aWantedType,
                                       nsCertCompareFunc  aCertCmpFn,
                                       void *aCertCmpFnArg)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("GetCertsByTypeFromCertList"));
  if (!aCertList)
    return NS_ERROR_FAILURE;

  nsCertOverrideService *cos = 
    reinterpret_cast<nsCertOverrideService*>(mOverrideService.get());
  if (!cos)
    return NS_ERROR_FAILURE;

  nsTHashtable<nsCStringHashKey> allHostPortOverrideKeys;
  if (!allHostPortOverrideKeys.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  if (aWantedType == nsIX509Cert::SERVER_CERT) {
    cos->EnumerateCertOverrides(nsnull, 
                                CollectAllHostPortOverridesCallback, 
                                &allHostPortOverrideKeys);
  }

  CERTCertListNode *node;
  int count = 0;
  for (node = CERT_LIST_HEAD(aCertList);
       !CERT_LIST_END(node, aCertList);
       node = CERT_LIST_NEXT(node)) {

    PRBool wantThisCert = (aWantedType == nsIX509Cert2::ANY_CERT);
    PRBool wantThisCertIfNoOverrides = PR_FALSE;
    PRBool wantThisCertIfHaveOverrides = PR_FALSE;
    PRBool addOverrides = PR_FALSE;

    if (!wantThisCert) {
      PRUint32 thisCertType = getCertType(node->cert);

      
      
      
      
      
      
      
      
      

      if (aWantedType == nsIX509Cert::SERVER_CERT
          && thisCertType == nsIX509Cert::UNKNOWN_CERT) {
        
        
        
        addOverrides = PR_TRUE;
      }
      else
      if (aWantedType == nsIX509Cert::UNKNOWN_CERT
          && thisCertType == nsIX509Cert::UNKNOWN_CERT) {
        
        
        
        wantThisCertIfNoOverrides = PR_TRUE;
      }
      else
      if (aWantedType == nsIX509Cert::SERVER_CERT
          && thisCertType == nsIX509Cert::SERVER_CERT) {
        
        
        wantThisCert = PR_TRUE;
        
        
        addOverrides = PR_TRUE;
      }
      else
      if (aWantedType == nsIX509Cert::SERVER_CERT
          && thisCertType == nsIX509Cert::EMAIL_CERT) {
        
        
        
        
        
        
        addOverrides = PR_TRUE;
      }
      else
      if (aWantedType == nsIX509Cert::EMAIL_CERT
          && thisCertType == nsIX509Cert::EMAIL_CERT) {
        
        
        
        
        
        
        wantThisCertIfNoOverrides = PR_TRUE;
      }
      else
      if (thisCertType == aWantedType) {
        wantThisCert = PR_TRUE;
      }
    }

    nsCOMPtr<nsIX509Cert> pipCert = nsNSSCertificate::Create(node->cert);
    if (!pipCert)
      return NS_ERROR_OUT_OF_MEMORY;

    if (wantThisCertIfNoOverrides || wantThisCertIfHaveOverrides) {
      PRUint32 ocount = 0;
      nsresult rv = 
        mOverrideService->IsCertUsedForOverrides(pipCert, 
                                                 PR_TRUE, 
                                                 PR_TRUE, 
                                                 &ocount);
      if (wantThisCertIfNoOverrides) {
        if (NS_FAILED(rv) || ocount == 0) {
          
          wantThisCert = PR_TRUE;
        }
      }

      if (wantThisCertIfHaveOverrides) {
        if (NS_SUCCEEDED(rv) && ocount > 0) {
          
          wantThisCert = PR_TRUE;
        }
      }
    }

    nsRefPtr<nsCertAddonInfo> certai = new nsCertAddonInfo;
    if (!certai)
      return NS_ERROR_OUT_OF_MEMORY;

    certai->mCert = pipCert;
    certai->mUsageCount = 0;

    if (wantThisCert || addOverrides) {
      int InsertPosition = 0;
      for (; InsertPosition < count; ++InsertPosition) {
        nsCOMPtr<nsIX509Cert> cert = nsnull;
        nsRefPtr<nsCertTreeDispInfo> elem = mDispInfo.ElementAt(InsertPosition);
        if (elem->mAddonInfo) {
          cert = mDispInfo.ElementAt(InsertPosition)->mAddonInfo->mCert;
        }
        if ((*aCertCmpFn)(aCertCmpFnArg, pipCert, cert) < 0) {
          break;
        }
      }
      if (wantThisCert) {
        nsCertTreeDispInfo *certdi = new nsCertTreeDispInfo;
        if (!certdi)
          return NS_ERROR_OUT_OF_MEMORY;

        certdi->mAddonInfo = certai;
        certai->mUsageCount++;
        certdi->mTypeOfEntry = nsCertTreeDispInfo::direct_db;
        
        certdi->mOverrideBits = nsCertOverride::ob_None;
        certdi->mIsTemporary = PR_FALSE;
        mDispInfo.InsertElementAt(InsertPosition, certdi);
        ++count;
        ++InsertPosition;
      }
      if (addOverrides && cos) {
        nsCertAndArrayAndPositionAndCounterAndTracker cap;
        cap.certai = certai;
        cap.array = &mDispInfo;
        cap.position = InsertPosition;
        cap.counter = 0;
        cap.tracker = &allHostPortOverrideKeys;

        cos->EnumerateCertOverrides(pipCert, MatchingCertOverridesCallback, &cap);
        count += cap.counter;
      }
    }
  }

  if (aWantedType == nsIX509Cert::SERVER_CERT) {
    nsArrayAndPositionAndCounterAndTracker cap;
    cap.array = &mDispInfo;
    cap.position = 0;
    cap.counter = 0;
    cap.tracker = &allHostPortOverrideKeys;
    cos->EnumerateCertOverrides(nsnull, AddRemaningHostPortOverridesCallback, &cap);
  }

  return NS_OK;
}

nsresult 
nsCertTree::GetCertsByType(PRUint32           aType,
                           nsCertCompareFunc  aCertCmpFn,
                           void              *aCertCmpFnArg)
{
  nsNSSShutDownPreventionLock locker;
  CERTCertList *certList = NULL;
  nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();
  certList = PK11_ListCerts(PK11CertListUnique, cxt);
  nsresult rv = GetCertsByTypeFromCertList(certList, aType, aCertCmpFn, aCertCmpFnArg);
  if (certList)
    CERT_DestroyCertList(certList);
  return rv;
}

nsresult 
nsCertTree::GetCertsByTypeFromCache(nsINSSCertCache   *aCache,
                                    PRUint32           aType,
                                    nsCertCompareFunc  aCertCmpFn,
                                    void              *aCertCmpFnArg)
{
  NS_ENSURE_ARG_POINTER(aCache);
  CERTCertList *certList = reinterpret_cast<CERTCertList*>(aCache->GetCachedCerts());
  if (!certList)
    return NS_ERROR_FAILURE;
  return GetCertsByTypeFromCertList(certList, aType, aCertCmpFn, aCertCmpFnArg);
}





NS_IMETHODIMP 
nsCertTree::LoadCertsFromCache(nsINSSCertCache *aCache, PRUint32 aType)
{
  if (mTreeArray) {
    FreeCertArray();
    delete [] mTreeArray;
    mTreeArray = nsnull;
    mNumRows = 0;
  }
  nsresult rv = InitCompareHash();
  if (NS_FAILED(rv)) return rv;

  rv = GetCertsByTypeFromCache(aCache, aType, 
                               GetCompareFuncFromCertType(aType), &mCompareCache);
  if (NS_FAILED(rv)) return rv;
  return UpdateUIContents();
}

NS_IMETHODIMP 
nsCertTree::LoadCerts(PRUint32 aType)
{
  if (mTreeArray) {
    FreeCertArray();
    delete [] mTreeArray;
    mTreeArray = nsnull;
    mNumRows = 0;
  }
  nsresult rv = InitCompareHash();
  if (NS_FAILED(rv)) return rv;

  rv = GetCertsByType(aType, 
                      GetCompareFuncFromCertType(aType), &mCompareCache);
  if (NS_FAILED(rv)) return rv;
  return UpdateUIContents();
}

nsresult
nsCertTree::UpdateUIContents()
{
  PRUint32 count = mDispInfo.Length();
  mNumOrgs = CountOrganizations();
  mTreeArray = new treeArrayEl[mNumOrgs];
  if (!mTreeArray)
    return NS_ERROR_OUT_OF_MEMORY;

  mCellText = do_CreateInstance(NS_ARRAY_CONTRACTID);

if (count) {
  PRUint32 j = 0;
  nsCOMPtr<nsIX509Cert> orgCert = nsnull;
  if (mDispInfo.ElementAt(j)->mAddonInfo) {
    orgCert = mDispInfo.ElementAt(j)->mAddonInfo->mCert;
  }
  for (PRInt32 i=0; i<mNumOrgs; i++) {
    nsString &orgNameRef = mTreeArray[i].orgName;
    if (!orgCert) {
      mNSSComponent->GetPIPNSSBundleString("CertOrgUnknown", orgNameRef);
    }
    else {
      orgCert->GetIssuerOrganization(orgNameRef);
      if (orgNameRef.IsEmpty())
        orgCert->GetCommonName(orgNameRef);
    }
    mTreeArray[i].open = PR_TRUE;
    mTreeArray[i].certIndex = j;
    mTreeArray[i].numChildren = 1;
    if (++j >= count) break;
    nsCOMPtr<nsIX509Cert> nextCert = nsnull;
    if (mDispInfo.ElementAt(j)->mAddonInfo) {
      nextCert = mDispInfo.ElementAt(j)->mAddonInfo->mCert;
    }
    while (0 == CmpBy(&mCompareCache, orgCert, nextCert, sort_IssuerOrg, sort_None, sort_None)) {
      mTreeArray[i].numChildren++;
      if (++j >= count) break;
      nextCert = nsnull;
      if (mDispInfo.ElementAt(j)->mAddonInfo) {
        nextCert = mDispInfo.ElementAt(j)->mAddonInfo->mCert;
      }
    }
    orgCert = nextCert;
  }
}
  if (mTree) {
    mTree->BeginUpdateBatch();
    mTree->RowCountChanged(0, -mNumRows);
  }
  mNumRows = count + mNumOrgs;
  if (mTree)
    mTree->EndUpdateBatch();
  return NS_OK;
}

NS_IMETHODIMP 
nsCertTree::DeleteEntryObject(PRUint32 index)
{
  if (!mTreeArray) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIX509CertDB> certdb = 
    do_GetService("@mozilla.org/security/x509certdb;1");
  if (!certdb) {
    return NS_ERROR_FAILURE;
  }

  int i;
  PRUint32 idx = 0, cIndex = 0, nc;
  
  for (i=0; i<mNumOrgs; i++) {
    if (index == idx)
      return NS_OK; 
    idx++; 
    nc = (mTreeArray[i].open) ? mTreeArray[i].numChildren : 0;
    if (index < idx + nc) { 
      PRInt32 certIndex = cIndex + index - idx;

      nsRefPtr<nsCertTreeDispInfo> certdi = mDispInfo.ElementAt(certIndex);
      nsCOMPtr<nsIX509Cert> cert = nsnull;
      if (certdi->mAddonInfo) {
        cert = certdi->mAddonInfo->mCert;
      }
      PRBool canRemoveEntry = PR_FALSE;

      if (certdi->mTypeOfEntry == nsCertTreeDispInfo::host_port_override) {
        mOverrideService->ClearValidityOverride(certdi->mAsciiHost, certdi->mPort);
        if (certdi->mAddonInfo) {
          certdi->mAddonInfo->mUsageCount--;
          if (certdi->mAddonInfo->mUsageCount == 0) {
            
            
            
            
            canRemoveEntry = PR_TRUE;
          }
        } 
      }
      else {
        if (certdi->mAddonInfo->mUsageCount > 1) {
          
          
          

          CERTCertificate *nsscert = nsnull;
          CERTCertificateCleaner nsscertCleaner(nsscert);

          nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(cert);
          if (cert2) {
            nsscert = cert2->GetCert();
          }

          if (nsscert) {
            CERTCertTrust trust;
            memset((void*)&trust, 0, sizeof(trust));
          
            SECStatus srv = CERT_DecodeTrustString(&trust, ""); 
            if (srv == SECSuccess) {
              CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), nsscert, &trust);
            }
          }
        }
        else {
          canRemoveEntry = PR_TRUE;
        }
      }

      mDispInfo.RemoveElementAt(certIndex);

      if (canRemoveEntry) {
        RemoveCacheEntry(cert);
        certdb->DeleteCertificate(cert);
      }

      delete [] mTreeArray;
      mTreeArray = nsnull;
      return UpdateUIContents();
    }
    if (mTreeArray[i].open)
      idx += mTreeArray[i].numChildren;
    cIndex += mTreeArray[i].numChildren;
    if (idx > index)
      break;
  }
  return NS_ERROR_FAILURE;
}








NS_IMETHODIMP
nsCertTree::GetCert(PRUint32 aIndex, nsIX509Cert **_cert)
{
  NS_ENSURE_ARG(_cert);
  *_cert = GetCertAtIndex(aIndex).get();
  return NS_OK;
}

NS_IMETHODIMP
nsCertTree::GetTreeItem(PRUint32 aIndex, nsICertTreeItem **_treeitem)
{
  NS_ENSURE_ARG(_treeitem);

  nsRefPtr<nsCertTreeDispInfo> certdi = 
    GetDispInfoAtIndex(aIndex);
  if (!certdi)
    return NS_ERROR_FAILURE;

  *_treeitem = certdi;
  NS_IF_ADDREF(*_treeitem);
  return NS_OK;
}

NS_IMETHODIMP
nsCertTree::IsHostPortOverride(PRUint32 aIndex, PRBool *_retval)
{
  NS_ENSURE_ARG(_retval);

  nsRefPtr<nsCertTreeDispInfo> certdi = 
    GetDispInfoAtIndex(aIndex);
  if (!certdi)
    return NS_ERROR_FAILURE;

  *_retval = (certdi->mTypeOfEntry == nsCertTreeDispInfo::host_port_override);
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetRowCount(PRInt32 *aRowCount)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  PRUint32 count = 0;
  for (PRInt32 i=0; i<mNumOrgs; i++) {
    if (mTreeArray[i].open) {
      count += mTreeArray[i].numChildren;
    }
    count++;
  }
  *aRowCount = count;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetSelection(nsITreeSelection * *aSelection)
{
  *aSelection = mSelection;
  NS_IF_ADDREF(*aSelection);
  return NS_OK;
}

NS_IMETHODIMP 
nsCertTree::SetSelection(nsITreeSelection * aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetRowProperties(PRInt32 index, nsISupportsArray *properties)
{
  return NS_OK;
}




NS_IMETHODIMP 
nsCertTree::GetCellProperties(PRInt32 row, nsITreeColumn* col, 
                              nsISupportsArray* properties)
{
  return NS_OK;
}




NS_IMETHODIMP 
nsCertTree::GetColumnProperties(nsITreeColumn* col, 
                                nsISupportsArray* properties)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsContainer(PRInt32 index, PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) {
    *_retval = PR_TRUE;
  } else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsContainerOpen(PRInt32 index, PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el && el->open) {
    *_retval = PR_TRUE;
  } else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsContainerEmpty(PRInt32 index, PRBool *_retval)
{
  *_retval = !mTreeArray;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsSeparator(PRInt32 index, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetParentIndex(PRInt32 rowIndex, PRInt32 *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  int i, idx = 0;
  for (i = 0; i < mNumOrgs && idx < rowIndex; i++, idx++) {
    if (mTreeArray[i].open) {
      if (rowIndex <= idx + mTreeArray[i].numChildren) {
        *_retval = idx;
        return NS_OK;
      }
      idx += mTreeArray[i].numChildren;
    }
  }
  *_retval = -1;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::HasNextSibling(PRInt32 rowIndex, PRInt32 afterIndex, 
                               PRBool *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;

  int i, idx = 0;
  for (i = 0; i < mNumOrgs && idx <= rowIndex; i++, idx++) {
    if (mTreeArray[i].open) {
      idx += mTreeArray[i].numChildren;
      if (afterIndex <= idx) {
        *_retval = afterIndex < idx;
        return NS_OK;
      }
    }
  }
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetLevel(PRInt32 index, PRInt32 *_retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) {
    *_retval = 0;
  } else {
    *_retval = 1;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetImageSrc(PRInt32 row, nsITreeColumn* col, 
                        nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
nsCertTree::GetProgressMode(PRInt32 row, nsITreeColumn* col, PRInt32* _retval)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetCellValue(PRInt32 row, nsITreeColumn* col, 
                         nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::GetCellText(PRInt32 row, nsITreeColumn* col, 
                        nsAString& _retval)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv;
  _retval.Truncate();

  const PRUnichar* colID;
  col->GetIdConst(&colID);

  treeArrayEl *el = GetThreadDescAtIndex(row);
  if (el != nsnull) {
    if (NS_LITERAL_STRING("certcol").Equals(colID))
      _retval.Assign(el->orgName);
    else
      _retval.Truncate();
    return NS_OK;
  }

  PRInt32 absoluteCertOffset;
  nsRefPtr<nsCertTreeDispInfo> certdi = 
    GetDispInfoAtIndex(row, &absoluteCertOffset);
  if (!certdi)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIX509Cert> cert = certdi->mCert;
  if (!cert && certdi->mAddonInfo) {
    cert = certdi->mAddonInfo->mCert;
  }

  PRInt32 colIndex;
  col->GetIndex(&colIndex);
  PRUint32 arrayIndex=absoluteCertOffset+colIndex*(mNumRows-mNumOrgs);
  PRUint32 arrayLength=0;
  if (mCellText) {
    mCellText->GetLength(&arrayLength);
  }
  if (arrayIndex < arrayLength) {
    nsCOMPtr<nsISupportsString> myString(do_QueryElementAt(mCellText, arrayIndex));
    if (myString) {
      myString->GetData(_retval);
      return NS_OK;
    }
  }

  if (NS_LITERAL_STRING("certcol").Equals(colID)) {
    if (!cert) {
      mNSSComponent->GetPIPNSSBundleString("CertNotStored", _retval);
    }
    else {
      rv = cert->GetCommonName(_retval);
      if (NS_FAILED(rv) || _retval.IsEmpty()) {
        
        
        nsAutoString nick;
        rv = cert->GetNickname(nick);
        
        nsAString::const_iterator start, end, end2;
        nick.BeginReading(start);
        nick.EndReading(end);
        end2 = end;
  
        if (FindInReadable(NS_LITERAL_STRING(":"), start, end)) {
          
          
          _retval = Substring(end, end2);
        }
        else {
          _retval = nick;
        }
      }
    }
  } else if (NS_LITERAL_STRING("tokencol").Equals(colID) && cert) {
    rv = cert->GetTokenName(_retval);
  } else if (NS_LITERAL_STRING("emailcol").Equals(colID) && cert) {
    rv = cert->GetEmailAddress(_retval);
  } else if (NS_LITERAL_STRING("purposecol").Equals(colID) && mNSSComponent && cert) {
    PRUint32 verified;

    nsAutoString theUsages;
    rv = cert->GetUsagesString(PR_FALSE, &verified, theUsages); 
    if (NS_FAILED(rv)) {
      verified = nsIX509Cert::NOT_VERIFIED_UNKNOWN;
    }

    switch (verified) {
      case nsIX509Cert::VERIFIED_OK:
        _retval = theUsages;
        break;

      case nsIX509Cert::CERT_REVOKED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyRevoked", _retval);
        break;
      case nsIX509Cert::CERT_EXPIRED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyExpired", _retval);
        break;
      case nsIX509Cert::CERT_NOT_TRUSTED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyNotTrusted", _retval);
        break;
      case nsIX509Cert::ISSUER_NOT_TRUSTED:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyIssuerNotTrusted", _retval);
        break;
      case nsIX509Cert::ISSUER_UNKNOWN:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyIssuerUnknown", _retval);
        break;
      case nsIX509Cert::INVALID_CA:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyInvalidCA", _retval);
        break;
      case nsIX509Cert::NOT_VERIFIED_UNKNOWN:
      case nsIX509Cert::USAGE_NOT_ALLOWED:
      default:
        rv = mNSSComponent->GetPIPNSSBundleString("VerifyUnknown", _retval);
        break;
    }
  } else if (NS_LITERAL_STRING("issuedcol").Equals(colID) && cert) {
    nsCOMPtr<nsIX509CertValidity> validity;

    rv = cert->GetValidity(getter_AddRefs(validity));
    if (NS_SUCCEEDED(rv)) {
      validity->GetNotBeforeLocalDay(_retval);
    }
  } else if (NS_LITERAL_STRING("expiredcol").Equals(colID) && cert) {
    nsCOMPtr<nsIX509CertValidity> validity;

    rv = cert->GetValidity(getter_AddRefs(validity));
    if (NS_SUCCEEDED(rv)) {
      validity->GetNotAfterLocalDay(_retval);
    }
  } else if (NS_LITERAL_STRING("serialnumcol").Equals(colID) && cert) {
    rv = cert->GetSerialNumber(_retval);


  } else if (NS_LITERAL_STRING("overridetypecol").Equals(colID)) {
    
    nsCertOverride::OverrideBits ob = nsCertOverride::ob_Untrusted;
    if (certdi->mTypeOfEntry == nsCertTreeDispInfo::host_port_override) {
      ob = certdi->mOverrideBits;
    }
    nsCAutoString temp;
    nsCertOverride::convertBitsToString(ob, temp);
    _retval = NS_ConvertUTF8toUTF16(temp);
  } else if (NS_LITERAL_STRING("sitecol").Equals(colID)) {
    if (certdi->mTypeOfEntry == nsCertTreeDispInfo::host_port_override) {
      nsCAutoString hostPort;
      nsCertOverrideService::GetHostWithPort(certdi->mAsciiHost, certdi->mPort, hostPort);
      _retval = NS_ConvertUTF8toUTF16(hostPort);
    }
    else {
      _retval = NS_LITERAL_STRING("*");
    }
  } else if (NS_LITERAL_STRING("lifetimecol").Equals(colID)) {
    const char *stringID = 
      (certdi->mIsTemporary) ? "CertExceptionTemporary" : "CertExceptionPermanent";
    rv = mNSSComponent->GetPIPNSSBundleString(stringID, _retval);
  } else if (NS_LITERAL_STRING("typecol").Equals(colID) && cert) {
    nsCOMPtr<nsIX509Cert2> pipCert = do_QueryInterface(cert);
    PRUint32 type = nsIX509Cert::UNKNOWN_CERT;

    if (pipCert) {
	rv = pipCert->GetCertType(&type);
    }

    switch (type) {
    case nsIX509Cert::USER_CERT:
        rv = mNSSComponent->GetPIPNSSBundleString("CertUser", _retval);
	break;
    case nsIX509Cert::CA_CERT:
        rv = mNSSComponent->GetPIPNSSBundleString("CertCA", _retval);
	break;
    case nsIX509Cert::SERVER_CERT:
        rv = mNSSComponent->GetPIPNSSBundleString("CertSSL", _retval);
	break;
    case nsIX509Cert::EMAIL_CERT:
        rv = mNSSComponent->GetPIPNSSBundleString("CertEmail", _retval);
	break;
    default:
        rv = mNSSComponent->GetPIPNSSBundleString("CertUnknown", _retval);
	break;
    }

  } else {
    return NS_ERROR_FAILURE;
  }
  if (mCellText) {
    nsCOMPtr<nsISupportsString> text(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    text->SetData(_retval);
    mCellText->ReplaceElementAt(text, arrayIndex, PR_FALSE);
  }
  return rv;
}


NS_IMETHODIMP 
nsCertTree::SetTree(nsITreeBoxObject *tree)
{
  mTree = tree;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::ToggleOpenState(PRInt32 index)
{
  if (!mTreeArray)
    return NS_ERROR_NOT_INITIALIZED;
  treeArrayEl *el = GetThreadDescAtIndex(index);
  if (el) {
    el->open = !el->open;
    PRInt32 newChildren = (el->open) ? el->numChildren : -el->numChildren;
    if (mTree) mTree->RowCountChanged(index + 1, newChildren);
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::CycleHeader(nsITreeColumn* col)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::SelectionChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP 
nsCertTree::CycleCell(PRInt32 row, nsITreeColumn* col)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsEditable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::IsSelectable(PRInt32 row, nsITreeColumn* col, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::SetCellValue(PRInt32 row, nsITreeColumn* col, 
                         const nsAString& value)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::SetCellText(PRInt32 row, nsITreeColumn* col, 
                        const nsAString& value)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::PerformAction(const PRUnichar *action)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsCertTree::PerformActionOnRow(const PRUnichar *action, PRInt32 row)
{
  return NS_OK;
}




NS_IMETHODIMP 
nsCertTree::PerformActionOnCell(const PRUnichar *action, PRInt32 row, 
                                nsITreeColumn* col)
{
  return NS_OK;
}

#ifdef DEBUG_CERT_TREE
void
nsCertTree::dumpMap()
{
  for (int i=0; i<mNumOrgs; i++) {
    nsAutoString org(mTreeArray[i].orgName);
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("ORG[%s]", NS_LossyConvertUTF16toASCII(org).get()));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("OPEN[%d]", mTreeArray[i].open));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("INDEX[%d]", mTreeArray[i].certIndex));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NCHILD[%d]", mTreeArray[i].numChildren));
  }
  for (int i=0; i<mNumRows; i++) {
    treeArrayEl *el = GetThreadDescAtIndex(i);
    if (el != nsnull) {
      nsAutoString td(el->orgName);
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("thread desc[%d]: %s", i, NS_LossyConvertUTF16toASCII(td).get()));
    }
    nsCOMPtr<nsIX509Cert> ct = GetCertAtIndex(i);
    if (ct != nsnull) {
      PRUnichar *goo;
      ct->GetCommonName(&goo);
      nsAutoString doo(goo);
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("cert [%d]: %s", i, NS_LossyConvertUTF16toASCII(doo).get()));
    }
  }
}
#endif




NS_IMETHODIMP nsCertTree::CanDrop(PRInt32 index, PRInt32 orientation,
                                  nsIDOMDataTransfer* aDataTransfer, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  
  return NS_OK;
}





NS_IMETHODIMP nsCertTree::Drop(PRInt32 row, PRInt32 orient, nsIDOMDataTransfer* aDataTransfer)
{
  return NS_OK;
}







NS_IMETHODIMP nsCertTree::IsSorted(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

#define RETURN_NOTHING

void 
nsCertTree::CmpInitCriterion(nsIX509Cert *cert, CompareCacheHashEntry *entry,
                             sortCriterion crit, PRInt32 level)
{
  NS_ENSURE_TRUE( (cert!=0 && entry!=0), RETURN_NOTHING );

  entry->mCritInit[level] = PR_TRUE;
  nsXPIDLString &str = entry->mCrit[level];
  
  switch (crit) {
    case sort_IssuerOrg:
      cert->GetIssuerOrganization(str);
      if (str.IsEmpty())
        cert->GetCommonName(str);
      break;
    case sort_Org:
      cert->GetOrganization(str);
      break;
    case sort_Token:
      cert->GetTokenName(str);
      break;
    case sort_CommonName:
      cert->GetCommonName(str);
      break;
    case sort_IssuedDateDescending:
      {
        nsresult rv;
        nsCOMPtr<nsIX509CertValidity> validity;
        PRTime notBefore;

        rv = cert->GetValidity(getter_AddRefs(validity));
        if (NS_SUCCEEDED(rv)) {
          rv = validity->GetNotBefore(&notBefore);
        }

        if (NS_SUCCEEDED(rv)) {
          PRExplodedTime explodedTime;
          PR_ExplodeTime(notBefore, PR_GMTParameters, &explodedTime);
          char datebuf[20]; 
          if (0 != PR_FormatTime(datebuf, sizeof(datebuf), "%Y%m%d%H%M%S", &explodedTime)) {
            str = NS_ConvertASCIItoUTF16(nsDependentCString(datebuf));
          }
        }
      }
      break;
    case sort_Email:
      cert->GetEmailAddress(str);
      break;
    case sort_None:
    default:
      break;
  }
}

PRInt32
nsCertTree::CmpByCrit(nsIX509Cert *a, CompareCacheHashEntry *ace, 
                      nsIX509Cert *b, CompareCacheHashEntry *bce, 
                      sortCriterion crit, PRInt32 level)
{
  NS_ENSURE_TRUE( (a!=0 && ace!=0 && b!=0 && bce!=0), 0 );

  if (!ace->mCritInit[level]) {
    CmpInitCriterion(a, ace, crit, level);
  }

  if (!bce->mCritInit[level]) {
    CmpInitCriterion(b, bce, crit, level);
  }

  nsXPIDLString &str_a = ace->mCrit[level];
  nsXPIDLString &str_b = bce->mCrit[level];

  PRInt32 result;
  if (str_a && str_b)
    result = Compare(str_a, str_b, nsCaseInsensitiveStringComparator());
  else
    result = !str_a ? (!str_b ? 0 : -1) : 1;

  if (sort_IssuedDateDescending == crit)
    result *= -1; 

  return result;
}

PRInt32
nsCertTree::CmpBy(void *cache, nsIX509Cert *a, nsIX509Cert *b, 
                  sortCriterion c0, sortCriterion c1, sortCriterion c2)
{
  
  
  
  
  
  

  if (!a && !b)
    return 0;

  if (!a)
    return -1;

  if (!b)
    return 1;

  NS_ENSURE_TRUE( (cache!=0 && a!=0 && b!=0), 0 );

  CompareCacheHashEntry *ace = getCacheEntry(cache, a);
  CompareCacheHashEntry *bce = getCacheEntry(cache, b);

  PRInt32 cmp;
  cmp = CmpByCrit(a, ace, b, bce, c0, 0);
  if (cmp != 0)
    return cmp;

  if (c1 != sort_None) {
    cmp = CmpByCrit(a, ace, b, bce, c1, 1);
    if (cmp != 0)
      return cmp;
    
    if (c2 != sort_None) {
      return CmpByCrit(a, ace, b, bce, c2, 2);
    }
  }

  return cmp;
}

PRInt32
nsCertTree::CmpCACert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Org, sort_Token);
}

PRInt32
nsCertTree::CmpWebSiteCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_CommonName, sort_None);
}

PRInt32
nsCertTree::CmpUserCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Token, sort_IssuedDateDescending);
}

PRInt32
nsCertTree::CmpEmailCert(void *cache, nsIX509Cert *a, nsIX509Cert *b)
{
  
  return CmpBy(cache, a, b, sort_IssuerOrg, sort_Email, sort_CommonName);
}

