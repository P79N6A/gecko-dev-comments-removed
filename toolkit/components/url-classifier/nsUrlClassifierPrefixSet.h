





#ifndef nsUrlClassifierPrefixSet_h_
#define nsUrlClassifierPrefixSet_h_

#include "nsISupportsUtils.h"
#include "nsID.h"
#include "nsIFile.h"
#include "nsIMemoryReporter.h"
#include "nsIMutableArray.h"
#include "nsIUrlClassifierPrefixSet.h"
#include "nsTArray.h"
#include "nsToolkitCompsCID.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "mozilla/FileUtils.h"

class nsUrlClassifierPrefixSet MOZ_FINAL
  : public nsIUrlClassifierPrefixSet
  , public nsIMemoryReporter
{
public:
  nsUrlClassifierPrefixSet();

  NS_IMETHOD Init(const nsACString& aName);
  NS_IMETHOD SetPrefixes(const uint32_t* aArray, uint32_t aLength);
  NS_IMETHOD GetPrefixes(uint32_t* aCount, uint32_t** aPrefixes);
  NS_IMETHOD Contains(uint32_t aPrefix, bool* aFound);
  NS_IMETHOD IsEmpty(bool* aEmpty);
  NS_IMETHOD LoadFromFile(nsIFile* aFile);
  NS_IMETHOD StoreToFile(nsIFile* aFile);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);

protected:
  virtual ~nsUrlClassifierPrefixSet();

  static const uint32_t DELTAS_LIMIT = 120;
  static const uint32_t MAX_INDEX_DIFF = (1 << 16);
  static const uint32_t PREFIXSET_VERSION_MAGIC = 1;

  nsresult MakePrefixSet(const uint32_t* aArray, uint32_t aLength);
  uint32_t BinSearch(uint32_t start, uint32_t end, uint32_t target);
  nsresult LoadFromFd(mozilla::AutoFDClose& fileFd);
  nsresult StoreToFd(mozilla::AutoFDClose& fileFd);

  
  
  bool mHasPrefixes;
  
  
  nsTArray<uint32_t> mIndexPrefixes;
  
  
  
  
  nsTArray<nsTArray<uint16_t> > mIndexDeltas;
  
  uint32_t mTotalPrefixes;

  nsCString mMemoryReportPath;
};

#endif
