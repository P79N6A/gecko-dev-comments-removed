





#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIFile.h"
#include "nsIMutableArray.h"
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

  NS_IMETHOD Init(const nsACString& aName);
  NS_IMETHOD SetPrefixes(const PRUint32* aArray, PRUint32 aLength);
  NS_IMETHOD GetPrefixes(PRUint32* aCount, PRUint32** aPrefixes);
  NS_IMETHOD Contains(PRUint32 aPrefix, bool* aFound);
  NS_IMETHOD IsEmpty(bool* aEmpty);
  NS_IMETHOD LoadFromFile(nsIFile* aFile);
  NS_IMETHOD StoreToFile(nsIFile* aFile);

  NS_DECL_ISUPPORTS

  
  
  size_t SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf);

protected:
  static const PRUint32 DELTAS_LIMIT = 100;
  static const PRUint32 MAX_INDEX_DIFF = (1 << 16);
  static const PRUint32 PREFIXSET_VERSION_MAGIC = 1;

  nsRefPtr<nsPrefixSetReporter> mReporter;

  nsresult MakePrefixSet(const PRUint32* aArray, PRUint32 aLength);
  PRUint32 BinSearch(PRUint32 start, PRUint32 end, PRUint32 target);
  nsresult LoadFromFd(mozilla::AutoFDClose& fileFd);
  nsresult StoreToFd(mozilla::AutoFDClose& fileFd);

  
  
  bool mHasPrefixes;
  
  FallibleTArray<PRUint32> mIndexPrefixes;
  
  
  FallibleTArray<PRUint32> mIndexStarts;
  
  FallibleTArray<PRUint16> mDeltas;
};

#endif
