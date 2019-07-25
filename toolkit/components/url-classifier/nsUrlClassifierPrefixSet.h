





#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIFile.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsIMemoryReporter.h"
#include "nsToolkitCompsCID.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "mozilla/FileUtils.h"

class nsPrefixSetReporter;

class nsUrlClassifierPrefixSet : public nsIUrlClassifierPrefixSet
{
public:
  nsUrlClassifierPrefixSet();
  virtual ~nsUrlClassifierPrefixSet();

  NS_IMETHOD SetPrefixes(const PRUint32* aArray, PRUint32 aLength);
  NS_IMETHOD Probe(PRUint32 aPrefix, PRUint32 aKey, bool* aReady, bool* aFound);
  NS_IMETHOD IsEmpty(bool * aEmpty);
  NS_IMETHOD LoadFromFile(nsIFile* aFile);
  NS_IMETHOD StoreToFile(nsIFile* aFile);
  NS_IMETHOD GetKey(PRUint32* aKey);

  NS_DECL_ISUPPORTS

  
  
  size_t SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf);

protected:
  static const PRUint32 DELTAS_LIMIT = 100;
  static const PRUint32 MAX_INDEX_DIFF = (1 << 16);
  static const PRUint32 PREFIXSET_VERSION_MAGIC = 1;

  mozilla::Mutex mPrefixSetLock;
  mozilla::CondVar mSetIsReady;
  nsRefPtr<nsPrefixSetReporter> mReporter;

  nsresult Contains(PRUint32 aPrefix, bool* aFound);
  nsresult MakePrefixSet(const PRUint32* aArray, PRUint32 aLength);
  PRUint32 BinSearch(PRUint32 start, PRUint32 end, PRUint32 target);
  nsresult LoadFromFd(mozilla::AutoFDClose & fileFd);
  nsresult StoreToFd(mozilla::AutoFDClose & fileFd);
  nsresult InitKey();

  
  
  bool mHasPrefixes;
  
  PRUint32 mRandomKey;
  
  FallibleTArray<PRUint32> mIndexPrefixes;
  
  
  FallibleTArray<PRUint32> mIndexStarts;
  
  FallibleTArray<PRUint16> mDeltas;

};

#endif
