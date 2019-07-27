





#include "LulMain.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>  
#include <string>

#include "mozilla/Assertions.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/DebugOnly.h"

#include "LulCommonExt.h"
#include "LulElfExt.h"

#include "LulMainInt.h"

#include "platform-linux-lul.h"  


#define DEBUG_MAIN 0


namespace lul {

using std::string;
using std::vector;
using mozilla::DebugOnly;

















static const char* 
NameOf_DW_REG(int16_t aReg)
{
  switch (aReg) {
    case DW_REG_CFA:       return "cfa";
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    case DW_REG_INTEL_XBP: return "xbp";
    case DW_REG_INTEL_XSP: return "xsp";
    case DW_REG_INTEL_XIP: return "xip";
#elif defined(LUL_ARCH_arm)
    case DW_REG_ARM_R7:    return "r7";
    case DW_REG_ARM_R11:   return "r11";
    case DW_REG_ARM_R12:   return "r12";
    case DW_REG_ARM_R13:   return "r13";
    case DW_REG_ARM_R14:   return "r14";
    case DW_REG_ARM_R15:   return "r15";
#else
# error "Unsupported arch"
#endif
    default: return "???";
  }
}

static string
ShowRule(const char* aNewReg, LExpr aExpr)
{
  char buf[64];
  string res = string(aNewReg) + "=";
  switch (aExpr.mHow) {
    case LExpr::UNKNOWN:
      res += "Unknown";
      break;
    case LExpr::NODEREF:
      sprintf(buf, "%s+%d", NameOf_DW_REG(aExpr.mReg), (int)aExpr.mOffset);
      res += buf;
      break;
    case LExpr::DEREF:
      sprintf(buf, "*(%s+%d)", NameOf_DW_REG(aExpr.mReg), (int)aExpr.mOffset);
      res += buf;
      break;
    default:
      res += "???";
      break;
  }
  return res;
}

void
RuleSet::Print(void(*aLog)(const char*))
{
  char buf[96];
  sprintf(buf, "[%llx .. %llx]: let ",
          (unsigned long long int)mAddr,
          (unsigned long long int)(mAddr + mLen - 1));
  string res = string(buf);
  res += ShowRule("cfa", mCfaExpr);
  res += " in";
  
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
  res += ShowRule(" RA", mXipExpr);
  res += ShowRule(" SP", mXspExpr);
  res += ShowRule(" BP", mXbpExpr);
#elif defined(LUL_ARCH_arm)
  res += ShowRule(" R15", mR15expr);
  res += ShowRule(" R7",  mR7expr);
  res += ShowRule(" R11", mR11expr);
  res += ShowRule(" R12", mR12expr);
  res += ShowRule(" R13", mR13expr);
  res += ShowRule(" R14", mR14expr);
#else
# error "Unsupported arch"
#endif
  aLog(res.c_str());
}

LExpr*
RuleSet::ExprForRegno(DW_REG_NUMBER aRegno) {
  switch (aRegno) {
    case DW_REG_CFA: return &mCfaExpr;
#   if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    case DW_REG_INTEL_XIP: return &mXipExpr;
    case DW_REG_INTEL_XSP: return &mXspExpr;
    case DW_REG_INTEL_XBP: return &mXbpExpr;
#   elif defined(LUL_ARCH_arm)
    case DW_REG_ARM_R15:   return &mR15expr;
    case DW_REG_ARM_R14:   return &mR14expr;
    case DW_REG_ARM_R13:   return &mR13expr;
    case DW_REG_ARM_R12:   return &mR12expr;
    case DW_REG_ARM_R11:   return &mR11expr;
    case DW_REG_ARM_R7:    return &mR7expr;
#   else
#     error "Unknown arch"
#   endif
    default: return nullptr;
  }
}

RuleSet::RuleSet()
{
  mAddr = 0;
  mLen  = 0;
  
  
}








SecMap::SecMap(void(*aLog)(const char*))
  : mSummaryMinAddr(1)
  , mSummaryMaxAddr(0)
  , mUsable(true)
  , mLog(aLog)
{}

SecMap::~SecMap() {
  mRuleSets.clear();
}


RuleSet*
SecMap::FindRuleSet(uintptr_t ia) {
  
  
  
  

  
  MOZ_ASSERT(mUsable);

  long int lo = 0;
  long int hi = (long int)mRuleSets.size() - 1;
  while (true) {
    
    if (lo > hi) {
      
      return nullptr;
    }
    long int  mid         = lo + ((hi - lo) / 2);
    RuleSet*  mid_ruleSet = &mRuleSets[mid];
    uintptr_t mid_minAddr = mid_ruleSet->mAddr;
    uintptr_t mid_maxAddr = mid_minAddr + mid_ruleSet->mLen - 1;
    if (ia < mid_minAddr) { hi = mid-1; continue; }
    if (ia > mid_maxAddr) { lo = mid+1; continue; }
    MOZ_ASSERT(mid_minAddr <= ia && ia <= mid_maxAddr);
    return mid_ruleSet;
  }
  
}



void
SecMap::AddRuleSet(RuleSet* rs) {
  mUsable = false;
  mRuleSets.push_back(*rs);
}


static bool
CmpRuleSetsByAddrLE(const RuleSet& rs1, const RuleSet& rs2) {
  return rs1.mAddr < rs2.mAddr;
}



void
SecMap::PrepareRuleSets(uintptr_t aStart, size_t aLen)
{
  if (mRuleSets.empty()) {
    return;
  }

  MOZ_ASSERT(aLen > 0);
  if (aLen == 0) {
    
    mRuleSets.clear();
    return;
  }

  
  std::sort(mRuleSets.begin(), mRuleSets.end(), CmpRuleSetsByAddrLE);

  
  
  for (size_t i = 0; i < mRuleSets.size(); ++i) {
    RuleSet* rs = &mRuleSets[i];
    if (rs->mLen > 0 &&
        (rs->mAddr < aStart || rs->mAddr + rs->mLen > aStart + aLen)) {
      rs->mLen = 0;
    }
  }

  
  
  
  
  while (true) {
    size_t i;
    size_t n = mRuleSets.size();
    size_t nZeroLen = 0;

    if (n == 0) {
      break;
    }

    for (i = 1; i < n; ++i) {
      RuleSet* prev = &mRuleSets[i-1];
      RuleSet* here = &mRuleSets[i];
      MOZ_ASSERT(prev->mAddr <= here->mAddr);
      if (prev->mAddr + prev->mLen > here->mAddr) {
        prev->mLen = here->mAddr - prev->mAddr;
      }
      if (prev->mLen == 0)
        nZeroLen++;
    }

    if (mRuleSets[n-1].mLen == 0) {
      nZeroLen++;
    }

    
    
    if (nZeroLen == 0) {
      break;
    }

    
    size_t j = 0;  
    for (i = 0; i < n; ++i) {
      if (mRuleSets[i].mLen == 0) {
        continue;
      }
      if (j != i) mRuleSets[j] = mRuleSets[i];
      ++j;
    }
    MOZ_ASSERT(i == n);
    MOZ_ASSERT(nZeroLen <= n);
    MOZ_ASSERT(j == n - nZeroLen);
    while (nZeroLen > 0) {
      mRuleSets.pop_back();
      nZeroLen--;
    }

    MOZ_ASSERT(mRuleSets.size() == j);
  }

  size_t n = mRuleSets.size();

#ifdef DEBUG
  
  
  if (n > 0) {
    MOZ_ASSERT(mRuleSets[0].mLen > 0);
    for (size_t i = 1; i < n; ++i) {
      RuleSet* prev = &mRuleSets[i-1];
      RuleSet* here = &mRuleSets[i];
      MOZ_ASSERT(prev->mAddr < here->mAddr);
      MOZ_ASSERT(here->mLen > 0);
      MOZ_ASSERT(prev->mAddr + prev->mLen <= here->mAddr);
    }
  }
#endif

  
  if (n == 0) {
    
    mSummaryMinAddr = 1;
    mSummaryMaxAddr = 0;
  } else {
    mSummaryMinAddr = mRuleSets[0].mAddr;
    mSummaryMaxAddr = mRuleSets[n-1].mAddr + mRuleSets[n-1].mLen - 1;
  }
  char buf[150];
  snprintf(buf, sizeof(buf),
           "PrepareRuleSets: %d entries, smin/smax 0x%llx, 0x%llx\n",
           (int)n, (unsigned long long int)mSummaryMinAddr,
                   (unsigned long long int)mSummaryMaxAddr);
  buf[sizeof(buf)-1] = 0;
  mLog(buf);

  
  mUsable = true;

  if (0) {
    mLog("\nRulesets after preening\n");
    for (size_t i = 0; i < mRuleSets.size(); ++i) {
      mRuleSets[i].Print(mLog);
      mLog("\n");
    }
    mLog("\n");
  }
}

bool SecMap::IsEmpty() {
  return mRuleSets.empty();
}















class SegArray {

 public:
  void add(uintptr_t lo, uintptr_t hi, bool val) {
    if (lo > hi) {
      return;
    }
    split_at(lo);
    if (hi < UINTPTR_MAX) {
      split_at(hi+1);
    }
    std::vector<Seg>::size_type iLo, iHi, i;
    iLo = find(lo);
    iHi = find(hi);
    for (i = iLo; i <= iHi; ++i) {
      mSegs[i].val = val;
    }
    preen();
  }

  
  bool getBoundingCodeSegment(uintptr_t* rx_min,
                              uintptr_t* rx_max, uintptr_t addr) {
    std::vector<Seg>::size_type i = find(addr);
    if (!mSegs[i].val) {
      return false;
    }
    *rx_min = mSegs[i].lo;
    *rx_max = mSegs[i].hi;
    return true;
  }

  SegArray() {
    Seg s(0, UINTPTR_MAX, false);
    mSegs.push_back(s);
  }

 private:
  struct Seg {
    Seg(uintptr_t lo, uintptr_t hi, bool val) : lo(lo), hi(hi), val(val) {}
    uintptr_t lo;
    uintptr_t hi;
    bool val;
  };

  void preen() {
    for (std::vector<Seg>::iterator iter = mSegs.begin();
         iter < mSegs.end()-1;
         ++iter) {
      if (iter[0].val != iter[1].val) {
        continue;
      }
      iter[0].hi = iter[1].hi;
      mSegs.erase(iter+1);
      
      
      --iter;
    }
  }

  
  std::vector<Seg>::size_type find(uintptr_t a) {
    long int lo = 0;
    long int hi = (long int)mSegs.size();
    while (true) {
      
      if (lo > hi) {
        
        return (std::vector<Seg>::size_type)(-1);
      }
      long int  mid    = lo + ((hi - lo) / 2);
      uintptr_t mid_lo = mSegs[mid].lo;
      uintptr_t mid_hi = mSegs[mid].hi;
      if (a < mid_lo) { hi = mid-1; continue; }
      if (a > mid_hi) { lo = mid+1; continue; }
      return (std::vector<Seg>::size_type)mid;
    }
  }

  void split_at(uintptr_t a) {
    std::vector<Seg>::size_type i = find(a);
    if (mSegs[i].lo == a) {
      return;
    }
    mSegs.insert( mSegs.begin()+i+1, mSegs[i] );
    mSegs[i].hi = a-1;
    mSegs[i+1].lo = a;
  }

  void show() {
    printf("<< %d entries:\n", (int)mSegs.size());
    for (std::vector<Seg>::iterator iter = mSegs.begin();
         iter < mSegs.end();
         ++iter) {
      printf("  %016llx  %016llx  %s\n",
             (unsigned long long int)(*iter).lo,
             (unsigned long long int)(*iter).hi,
             (*iter).val ? "true" : "false");
    }
    printf(">>\n");
  }

  std::vector<Seg> mSegs;
};






class PriMap {
 public:
  explicit PriMap(void (*aLog)(const char*))
    : mLog(aLog)
  {}

  ~PriMap() {
    for (std::vector<SecMap*>::iterator iter = mSecMaps.begin();
         iter != mSecMaps.end();
         ++iter) {
      delete *iter;
    }
    mSecMaps.clear();
  }

  
  RuleSet* Lookup(uintptr_t ia) {
    SecMap* sm = FindSecMap(ia);
    return sm ? sm->FindRuleSet(ia) : nullptr;
  }

  
  
  void AddSecMap(SecMap* aSecMap) {
    
    
    if (aSecMap->IsEmpty()) {
      return;
    }

    
    
    
    
    
    MOZ_ASSERT(aSecMap->mSummaryMinAddr <= aSecMap->mSummaryMaxAddr);

    size_t num_secMaps = mSecMaps.size();
    uintptr_t i;
    for (i = 0; i < num_secMaps; ++i) {
      SecMap* sm_i = mSecMaps[i];
      MOZ_ASSERT(sm_i->mSummaryMinAddr <= sm_i->mSummaryMaxAddr);
      if (aSecMap->mSummaryMinAddr < sm_i->mSummaryMaxAddr) {
        
        break;
      }
    }
    MOZ_ASSERT(i <= num_secMaps);
    if (i == num_secMaps) {
      
      mSecMaps.push_back(aSecMap);
    } else {
      std::vector<SecMap*>::iterator iter = mSecMaps.begin() + i;
      mSecMaps.insert(iter, aSecMap);
    }
    char buf[100];
    snprintf(buf, sizeof(buf), "AddSecMap: now have %d SecMaps\n",
             (int)mSecMaps.size());
    buf[sizeof(buf)-1] = 0;
    mLog(buf);
  }

  
  
  void RemoveSecMapsInRange(uintptr_t avma_min, uintptr_t avma_max) {
    MOZ_ASSERT(avma_min <= avma_max);
    size_t num_secMaps = mSecMaps.size();
    if (num_secMaps > 0) {
      intptr_t i;
      
      
      
      
      for (i = (intptr_t)num_secMaps-1; i >= 0; i--) {
        SecMap* sm_i = mSecMaps[i];
        if (sm_i->mSummaryMaxAddr < avma_min ||
            avma_max < sm_i->mSummaryMinAddr) {
          
          continue;
        }
        
        
        mSecMaps.erase(mSecMaps.begin() + i);
        delete sm_i;
      }
    }
  }

  
  size_t CountSecMaps() {
    return mSecMaps.size();
  }

  
  
  
  bool MaybeIsReturnPoint(TaggedUWord aInstrAddr, SegArray* aSegArray) {
    if (!aInstrAddr.Valid()) {
      return false;
    }

    uintptr_t ia = aInstrAddr.Value();

    
    
    if (ia < 4096 || ((uintptr_t)(-ia)) < 4096) {
      return false;
    }

    
    
    uintptr_t insns_min, insns_max;
    bool b = aSegArray->getBoundingCodeSegment(&insns_min, &insns_max, ia);
    if (!b) {
      
      return false;
    }

    
    

#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    
    
    
    
    
    
    
    
    

    uint8_t* p = (uint8_t*)ia;
#   if defined(LUL_ARCH_x64)
    
    if (ia - 6 >= insns_min && p[-6] == 0xFF && p[-5] == 0x15) {
      return true;
    }
#   endif
    
    if (ia - 5 >= insns_min && p[-5] == 0xE8) {
      return true;
    }
    
    
    
    if (ia - 2 >= insns_min &&
        p[-2] == 0xFF && p[-1] >= 0xD0 && p[-1] <= 0xD7) {
      return true;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (ia - 3 >= insns_min &&
        p[-3] == 0xFF &&
        (p[-2] >= 0x50 && p[-2] <= 0x57 && p[-2] != 0x54)) {
      
      return true;
    }
    if (ia - 4 >= insns_min &&
        p[-4] == 0xFF && p[-3] == 0x54 && p[-2] == 0x24) {
      
      return true;
    }
    if (ia - 6 >= insns_min &&
        p[-6] == 0xFF &&
        (p[-5] >= 0x90 && p[-5] <= 0x97 && p[-5] != 0x94)) {
      
      return true;
    }
    if (ia - 7 >= insns_min &&
        p[-7] == 0xFF && p[-6] == 0x94 && p[-5] == 0x24) {
      
      return true;
    }

#elif defined(LUL_ARCH_arm)
    if (ia & 1) {
      uint16_t w0 = 0, w1 = 0;
      
      
      ia &= ~(uintptr_t)1;
      if (ia - 2 >= insns_min && ia - 1 <= insns_max) {
        w1 = *(uint16_t*)(ia - 2);
      }
      if (ia - 4 >= insns_min && ia - 1 <= insns_max) {
        w0 = *(uint16_t*)(ia - 4);
      }
      
      
      if ((w0 & 0xF800) == 0xF000 && (w1 & 0xC000) == 0xC000) {
        return true;
      }
      
      if ((w0 & 0xF800) == 0xF000 && (w1 & 0xC000) == 0xC000) {
        return true;
      }
      
      
      
      
    } else {
      
      uint32_t a0 = 0;
      if ((ia & 3) == 0 && ia - 4 >= insns_min && ia - 1 <= insns_max) {
        a0 = *(uint32_t*)(ia - 4);
      }
      
      
      
      if ((a0 & 0xFF000000) == 0xEB000000) {
        return true;
      }
      
      
      
      
      
      
      
    }

#else
# error "Unsupported arch"
#endif

    
    return false;
  }

 private:
  
  SecMap* FindSecMap(uintptr_t ia) {
    
    
    
    long int lo = 0;
    long int hi = (long int)mSecMaps.size() - 1;
    while (true) {
      
      if (lo > hi) {
        
        return nullptr;
      }
      long int  mid         = lo + ((hi - lo) / 2);
      SecMap*   mid_secMap  = mSecMaps[mid];
      uintptr_t mid_minAddr = mid_secMap->mSummaryMinAddr;
      uintptr_t mid_maxAddr = mid_secMap->mSummaryMaxAddr;
      if (ia < mid_minAddr) { hi = mid-1; continue; }
      if (ia > mid_maxAddr) { lo = mid+1; continue; }
      MOZ_ASSERT(mid_minAddr <= ia && ia <= mid_maxAddr);
      return mid_secMap;
    }
    
  }

 private:
  
  std::vector<SecMap*> mSecMaps;

  
  void (*mLog)(const char*);
};






#define LUL_LOG(_str) \
  do { \
    char buf[200]; \
    snprintf(buf, sizeof(buf), \
             "LUL: pid %d tid %d lul-obj %p: %s", \
             getpid(), gettid(), this, (_str)); \
    buf[sizeof(buf)-1] = 0; \
    mLog(buf); \
  } while (0)

LUL::LUL(void (*aLog)(const char*))
  : mLog(aLog)
  , mAdminMode(true)
  , mAdminThreadId(gettid())
  , mPriMap(new PriMap(aLog))
  , mSegArray(new SegArray())
  , mUSU(new UniqueStringUniverse())
{
  LUL_LOG("LUL::LUL: Created object");
}


LUL::~LUL()
{
  LUL_LOG("LUL::~LUL: Destroyed object");
  delete mPriMap;
  delete mSegArray;
  mLog = nullptr;
  delete mUSU;
}


void
LUL::MaybeShowStats()
{
  
  
  
  
  
  uint32_t n_new = mStats - mStatsPrevious;
  if (n_new >= 5000) {
    uint32_t n_new_Context = mStats.mContext - mStatsPrevious.mContext;
    uint32_t n_new_CFI     = mStats.mCFI     - mStatsPrevious.mCFI;
    uint32_t n_new_Scanned = mStats.mScanned - mStatsPrevious.mScanned;
    mStatsPrevious = mStats;
    char buf[200];
    snprintf(buf, sizeof(buf),
             "LUL frame stats: TOTAL %5u"
             "    CTX %4u    CFI %4u    SCAN %4u",
             n_new, n_new_Context, n_new_CFI, n_new_Scanned);
    buf[sizeof(buf)-1] = 0;
    mLog(buf);
  }
}


void
LUL::EnableUnwinding()
{
  LUL_LOG("LUL::EnableUnwinding");
  
  
  MOZ_ASSERT(gettid() == mAdminThreadId);

  mAdminMode = false;
}


void
LUL::NotifyAfterMap(uintptr_t aRXavma, size_t aSize,
                    const char* aFileName, const void* aMappedImage)
{
  MOZ_ASSERT(mAdminMode);
  MOZ_ASSERT(gettid() == mAdminThreadId);

  mLog(":\n");
  char buf[200];
  snprintf(buf, sizeof(buf), "NotifyMap %llx %llu %s\n",
           (unsigned long long int)aRXavma, (unsigned long long int)aSize,
           aFileName);
  buf[sizeof(buf)-1] = 0;
  mLog(buf);

  
  if (aSize > 0) {

    
    SecMap* smap = new SecMap(mLog);

    
    if (!aMappedImage) {
      (void)lul::ReadSymbolData(
              string(aFileName), std::vector<string>(), smap,
              (void*)aRXavma, aSize, mUSU, mLog);
    } else {
      (void)lul::ReadSymbolDataInternal(
              (const uint8_t*)aMappedImage,
              string(aFileName), std::vector<string>(), smap,
              (void*)aRXavma, aSize, mUSU, mLog);
    }

    mLog("NotifyMap .. preparing entries\n");

    smap->PrepareRuleSets(aRXavma, aSize);

    snprintf(buf, sizeof(buf),
             "NotifyMap got %lld entries\n", (long long int)smap->Size());
    buf[sizeof(buf)-1] = 0;
    mLog(buf);

    
    mPriMap->AddSecMap(smap);

    
    
    mSegArray->add(aRXavma, aRXavma + aSize - 1, true);
  }
}


void
LUL::NotifyExecutableArea(uintptr_t aRXavma, size_t aSize)
{
  MOZ_ASSERT(mAdminMode);
  MOZ_ASSERT(gettid() == mAdminThreadId);

  mLog(":\n");
  char buf[200];
  snprintf(buf, sizeof(buf), "NotifyExecutableArea %llx %llu\n",
           (unsigned long long int)aRXavma, (unsigned long long int)aSize);
  buf[sizeof(buf)-1] = 0;
  mLog(buf);

  
  if (aSize > 0) {
    
    
    mSegArray->add(aRXavma, aRXavma + aSize - 1, true);
  }
}


void
LUL::NotifyBeforeUnmap(uintptr_t aRXavmaMin, uintptr_t aRXavmaMax)
{
  MOZ_ASSERT(mAdminMode);
  MOZ_ASSERT(gettid() == mAdminThreadId);

  mLog(":\n");
  char buf[100];
  snprintf(buf, sizeof(buf), "NotifyUnmap %016llx-%016llx\n",
           (unsigned long long int)aRXavmaMin,
           (unsigned long long int)aRXavmaMax);
  buf[sizeof(buf)-1] = 0;
  mLog(buf);

  MOZ_ASSERT(aRXavmaMin <= aRXavmaMax);

  
  
  mPriMap->RemoveSecMapsInRange(aRXavmaMin, aRXavmaMax);

  
  
  mSegArray->add(aRXavmaMin, aRXavmaMax, false);

  snprintf(buf, sizeof(buf), "NotifyUnmap: now have %d SecMaps\n",
           (int)mPriMap->CountSecMaps());
  buf[sizeof(buf)-1] = 0;
  mLog(buf);
}


size_t
LUL::CountMappings()
{
  MOZ_ASSERT(mAdminMode);
  MOZ_ASSERT(gettid() == mAdminThreadId);

  return mPriMap->CountSecMaps();
}



static
TaggedUWord DerefTUW(TaggedUWord aAddr, StackImage* aStackImg)
{
  if (!aAddr.Valid()) {
    return TaggedUWord();
  }
  if (aAddr.Value() < aStackImg->mStartAvma) {
    return TaggedUWord();
  }
  if (aAddr.Value() + sizeof(uintptr_t) > aStackImg->mStartAvma
                                          + aStackImg->mLen) {
    return TaggedUWord();
  }
  return TaggedUWord(*(uintptr_t*)(aStackImg->mContents + aAddr.Value()
                                   - aStackImg->mStartAvma));
}

  
static
TaggedUWord EvaluateReg(int16_t aReg, UnwindRegs* aOldRegs, TaggedUWord aCFA)
{
  switch (aReg) {
    case DW_REG_CFA:       return aCFA;
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    case DW_REG_INTEL_XBP: return aOldRegs->xbp;
    case DW_REG_INTEL_XSP: return aOldRegs->xsp;
    case DW_REG_INTEL_XIP: return aOldRegs->xip;
#elif defined(LUL_ARCH_arm)
    case DW_REG_ARM_R7:    return aOldRegs->r7;
    case DW_REG_ARM_R11:   return aOldRegs->r11;
    case DW_REG_ARM_R12:   return aOldRegs->r12;
    case DW_REG_ARM_R13:   return aOldRegs->r13;
    case DW_REG_ARM_R14:   return aOldRegs->r14;
    case DW_REG_ARM_R15:   return aOldRegs->r15;
#else
# error "Unsupported arch"
#endif
    default: MOZ_ASSERT(0); return TaggedUWord();
  }
}


static
TaggedUWord EvaluateExpr(LExpr aExpr, UnwindRegs* aOldRegs,
                         TaggedUWord aCFA, StackImage* aStackImg)
{
  switch (aExpr.mHow) {
    case LExpr::UNKNOWN:
      return TaggedUWord();
    case LExpr::NODEREF: {
      TaggedUWord tuw = EvaluateReg(aExpr.mReg, aOldRegs, aCFA);
      tuw.Add(TaggedUWord((intptr_t)aExpr.mOffset));
      return tuw;
    }
    case LExpr::DEREF: {
      TaggedUWord tuw = EvaluateReg(aExpr.mReg, aOldRegs, aCFA);
      tuw.Add(TaggedUWord((intptr_t)aExpr.mOffset));
      return DerefTUW(tuw, aStackImg);
    }
    default:
      MOZ_ASSERT(0);
      return TaggedUWord();
  }
}


static
void UseRuleSet(UnwindRegs* aRegs,
                StackImage* aStackImg, RuleSet* aRS)
{
  
  
  UnwindRegs old_regs = *aRegs;

  
  
  
  
  
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
  aRegs->xbp = TaggedUWord();
  aRegs->xsp = TaggedUWord();
  aRegs->xip = TaggedUWord();
#elif defined(LUL_ARCH_arm)
  aRegs->r7  = TaggedUWord();
  aRegs->r11 = TaggedUWord();
  aRegs->r12 = TaggedUWord();
  aRegs->r13 = TaggedUWord();
  aRegs->r14 = TaggedUWord();
  aRegs->r15 = TaggedUWord();
#else
#  error "Unsupported arch"
#endif

  
  const TaggedUWord inval = TaggedUWord();

  
  TaggedUWord cfa = EvaluateExpr(aRS->mCfaExpr, &old_regs,
                                 inval, aStackImg);

  
  
  
  

#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
  aRegs->xbp = EvaluateExpr(aRS->mXbpExpr, &old_regs, cfa, aStackImg);
  aRegs->xsp = EvaluateExpr(aRS->mXspExpr, &old_regs, cfa, aStackImg);
  aRegs->xip = EvaluateExpr(aRS->mXipExpr, &old_regs, cfa, aStackImg);
#elif defined(LUL_ARCH_arm)
  aRegs->r7  = EvaluateExpr(aRS->mR7expr,  &old_regs, cfa, aStackImg);
  aRegs->r11 = EvaluateExpr(aRS->mR11expr, &old_regs, cfa, aStackImg);
  aRegs->r12 = EvaluateExpr(aRS->mR12expr, &old_regs, cfa, aStackImg);
  aRegs->r13 = EvaluateExpr(aRS->mR13expr, &old_regs, cfa, aStackImg);
  aRegs->r14 = EvaluateExpr(aRS->mR14expr, &old_regs, cfa, aStackImg);
  aRegs->r15 = EvaluateExpr(aRS->mR15expr, &old_regs, cfa, aStackImg);
#else
# error "Unsupported arch"
#endif

  
  
}


void
LUL::Unwind(uintptr_t* aFramePCs,
            uintptr_t* aFrameSPs,
            size_t* aFramesUsed, 
            size_t* aScannedFramesAcquired,
            size_t aFramesAvail,
            size_t aScannedFramesAllowed,
            UnwindRegs* aStartRegs, StackImage* aStackImg)
{
  MOZ_ASSERT(!mAdminMode);

  
  

  *aFramesUsed = 0;

  UnwindRegs  regs          = *aStartRegs;
  TaggedUWord last_valid_sp = TaggedUWord();

  
  unsigned int n_scanned_frames      = 0;  
  static const int NUM_SCANNED_WORDS = 50; 

  while (true) {

    if (DEBUG_MAIN) {
      char buf[300];
      mLog("\n");
#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
      snprintf(buf, sizeof(buf),
               "LoopTop: rip %d/%llx  rsp %d/%llx  rbp %d/%llx\n",
               (int)regs.xip.Valid(), (unsigned long long int)regs.xip.Value(),
               (int)regs.xsp.Valid(), (unsigned long long int)regs.xsp.Value(),
               (int)regs.xbp.Valid(), (unsigned long long int)regs.xbp.Value());
      buf[sizeof(buf)-1] = 0;
      mLog(buf);
#elif defined(LUL_ARCH_arm)
      snprintf(buf, sizeof(buf),
               "LoopTop: r15 %d/%llx  r7 %d/%llx  r11 %d/%llx"
               "  r12 %d/%llx  r13 %d/%llx  r14 %d/%llx\n",
               (int)regs.r15.Valid(), (unsigned long long int)regs.r15.Value(),
               (int)regs.r7.Valid(),  (unsigned long long int)regs.r7.Value(),
               (int)regs.r11.Valid(), (unsigned long long int)regs.r11.Value(),
               (int)regs.r12.Valid(), (unsigned long long int)regs.r12.Value(),
               (int)regs.r13.Valid(), (unsigned long long int)regs.r13.Value(),
               (int)regs.r14.Valid(), (unsigned long long int)regs.r14.Value());
      buf[sizeof(buf)-1] = 0;
      mLog(buf);
#else
# error "Unsupported arch"
#endif
    }

#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
    TaggedUWord ia = regs.xip;
    TaggedUWord sp = regs.xsp;
#elif defined(LUL_ARCH_arm)
    TaggedUWord ia = (*aFramesUsed == 0 ? regs.r15 : regs.r14);
    TaggedUWord sp = regs.r13;
#else
# error "Unsupported arch"
#endif

    if (*aFramesUsed >= aFramesAvail) {
      break;
    }

    
    if (!ia.Valid()) {
      break;
    }

    
    
    
    
    
    if (*aFramesUsed == 0) {
      last_valid_sp = sp;
    } else {
      MOZ_ASSERT(last_valid_sp.Valid());
      if (sp.Valid()) {
        if (sp.Value() < last_valid_sp.Value()) {
          
          break;
        }
        
        last_valid_sp = sp;
      }
    }

    
    
    
    aFramePCs[*aFramesUsed] = ia.Value() - (*aFramesUsed == 0 ? 0 : 1);
    aFrameSPs[*aFramesUsed] = sp.Valid() ? sp.Value() : 0;
    (*aFramesUsed)++;

    
    
    

    
    if (*aFramesUsed > 1) {
      ia.Add(TaggedUWord((uintptr_t)(-1)));
    }

    RuleSet* ruleset = mPriMap->Lookup(ia.Value());
    if (DEBUG_MAIN) {
      char buf[100];
      snprintf(buf, sizeof(buf), "ruleset for 0x%llx = %p\n",
               (unsigned long long int)ia.Value(), ruleset);
      buf[sizeof(buf)-1] = 0;
      mLog(buf);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
#if defined(LUL_PLAT_x86_android) || defined(LUL_PLAT_x86_linux)
    if (!ruleset && *aFramesUsed == 1 && ia.Valid() && sp.Valid()) {
      uintptr_t insns_min, insns_max;
      uintptr_t eip = ia.Value();
      bool b = mSegArray->getBoundingCodeSegment(&insns_min, &insns_max, eip);
      if (b && eip - 2 >= insns_min && eip + 3 <= insns_max) {
        uint8_t* eipC = (uint8_t*)eip;
        if (eipC[-2] == 0xCD && eipC[-1] == 0x80 && eipC[0] == 0x5D &&
            eipC[1] == 0x5A && eipC[2] == 0x59 && eipC[3] == 0xC3) {
          TaggedUWord sp_plus_0  = sp;
          TaggedUWord sp_plus_12 = sp;
          TaggedUWord sp_plus_16 = sp;
          sp_plus_12.Add(TaggedUWord(12));
          sp_plus_16.Add(TaggedUWord(16));
          TaggedUWord new_ebp = DerefTUW(sp_plus_0, aStackImg);
          TaggedUWord new_eip = DerefTUW(sp_plus_12, aStackImg);
          TaggedUWord new_esp = sp_plus_16;
          if (new_ebp.Valid() && new_eip.Valid() && new_esp.Valid()) {
            regs.xbp = new_ebp;
            regs.xip = new_eip;
            regs.xsp = new_esp;
            continue;
          }
        }
      }
    }
#endif
    
    

    
    if (ruleset) {

      if (DEBUG_MAIN) {
        ruleset->Print(mLog); mLog("\n");
      }
      
      
      UseRuleSet(&regs, aStackImg, ruleset);

    } else {

      
      

      
      if (n_scanned_frames++ >= aScannedFramesAllowed) {
        break;
      }

      
      if (!sp.IsAligned()) {
        break;
      }

      bool scan_succeeded = false;
      for (int i = 0; i < NUM_SCANNED_WORDS; ++i) {
        TaggedUWord aWord = DerefTUW(sp, aStackImg);
        
        
        if (!aWord.Valid()) {
          break;
        }

        
        
        if (mPriMap->MaybeIsReturnPoint(aWord, mSegArray)) {
          
          
          scan_succeeded = true;
          (*aScannedFramesAcquired)++;

#if defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
          
          
          
#         if defined(LUL_ARCH_x64)
          const int wordSize = 8;
#         else
          const int wordSize = 4;
#         endif
          
          
          
          
          regs.xsp = sp;
          regs.xsp.Add(TaggedUWord(wordSize));

          
          
          regs.xip = aWord;
          regs.xip.Add(TaggedUWord((uintptr_t)(-1)));

          
          if (regs.xbp.Valid() &&
              sp.Valid() && regs.xbp.Value() == sp.Value() - wordSize) {
            
            
            
            
            regs.xbp = DerefTUW(regs.xbp, aStackImg);
          } else if (regs.xbp.Valid() &&
                     sp.Valid() && regs.xbp.Value() >= sp.Value() + wordSize) {
            
            
            
            
            
            
          } else {
            
            
            regs.xbp = TaggedUWord();
          }

          
          sp.Add(TaggedUWord(wordSize));

#elif defined(LUL_ARCH_arm)
          
          

          
          
          
          
          regs.r15 = aWord;
          regs.r15.Add(TaggedUWord((uintptr_t)(-2)));

          
          
          regs.r13 = sp;
          regs.r13.Add(TaggedUWord(4));

          
          regs.r7 = regs.r11 = regs.r12 = regs.r14 = TaggedUWord();

          
          sp.Add(TaggedUWord(4));

#else
# error "Unknown plat"
#endif

          break;
        }

      } 

      
      
      if (!scan_succeeded) {
        break;
      }
    }

  } 

  
  
}






static const int LUL_UNIT_TEST_STACK_SIZE = 16384;













static __attribute__((noinline))
bool GetAndCheckStackTrace(LUL* aLUL, const char* dstring)
{
  
  UnwindRegs startRegs;
  memset(&startRegs, 0, sizeof(startRegs));
#if defined(LUL_PLAT_x64_linux)
  volatile uintptr_t block[3];
  MOZ_ASSERT(sizeof(block) == 24);
  __asm__ __volatile__(
    "leaq 0(%%rip), %%r15"   "\n\t"
    "movq %%r15, 0(%0)"      "\n\t"
    "movq %%rsp, 8(%0)"      "\n\t"
    "movq %%rbp, 16(%0)"     "\n"
    : : "r"(&block[0]) : "memory", "r15"
  );
  startRegs.xip = TaggedUWord(block[0]);
  startRegs.xsp = TaggedUWord(block[1]);
  startRegs.xbp = TaggedUWord(block[2]);
  const uintptr_t REDZONE_SIZE = 128;
  uintptr_t start = block[1] - REDZONE_SIZE;
#elif defined(LUL_PLAT_x86_linux) || defined(LUL_PLAT_x86_android)
  volatile uintptr_t block[3];
  MOZ_ASSERT(sizeof(block) == 12);
  __asm__ __volatile__(
    ".byte 0xE8,0x00,0x00,0x00,0x00"  "\n\t"
    "popl %%edi"             "\n\t"
    "movl %%edi, 0(%0)"      "\n\t"
    "movl %%esp, 4(%0)"      "\n\t"
    "movl %%ebp, 8(%0)"      "\n"
    : : "r"(&block[0]) : "memory", "edi"
  );
  startRegs.xip = TaggedUWord(block[0]);
  startRegs.xsp = TaggedUWord(block[1]);
  startRegs.xbp = TaggedUWord(block[2]);
  const uintptr_t REDZONE_SIZE = 0;
  uintptr_t start = block[1] - REDZONE_SIZE;
#elif defined(LUL_PLAT_arm_android)
  volatile uintptr_t block[6];
  MOZ_ASSERT(sizeof(block) == 24);
  __asm__ __volatile__(
    "mov r0, r15"            "\n\t"
    "str r0,  [%0, #0]"      "\n\t"
    "str r14, [%0, #4]"      "\n\t"
    "str r13, [%0, #8]"      "\n\t"
    "str r12, [%0, #12]"     "\n\t"
    "str r11, [%0, #16]"     "\n\t"
    "str r7,  [%0, #20]"     "\n"
    : : "r"(&block[0]) : "memory", "r0"
  );
  startRegs.r15 = TaggedUWord(block[0]);
  startRegs.r14 = TaggedUWord(block[1]);
  startRegs.r13 = TaggedUWord(block[2]);
  startRegs.r12 = TaggedUWord(block[3]);
  startRegs.r11 = TaggedUWord(block[4]);
  startRegs.r7  = TaggedUWord(block[5]);
  const uintptr_t REDZONE_SIZE = 0;
  uintptr_t start = block[1] - REDZONE_SIZE;
#else
# error "Unsupported platform"
#endif

  
  
  uintptr_t end = start + LUL_UNIT_TEST_STACK_SIZE;
  uintptr_t ws  = sizeof(void*);
  start &= ~(ws-1);
  end   &= ~(ws-1);
  uintptr_t nToCopy = end - start;
  if (nToCopy > lul::N_STACK_BYTES) {
    nToCopy = lul::N_STACK_BYTES;
  }
  MOZ_ASSERT(nToCopy <= lul::N_STACK_BYTES);
  StackImage* stackImg = new StackImage();
  stackImg->mLen       = nToCopy;
  stackImg->mStartAvma = start;
  if (nToCopy > 0) {
    MOZ_MAKE_MEM_DEFINED((void*)start, nToCopy);
    memcpy(&stackImg->mContents[0], (void*)start, nToCopy);
  }

  
  const int MAX_TEST_FRAMES = 64;
  uintptr_t framePCs[MAX_TEST_FRAMES];
  uintptr_t frameSPs[MAX_TEST_FRAMES];
  size_t framesAvail = mozilla::ArrayLength(framePCs);
  size_t framesUsed  = 0;
  size_t scannedFramesAllowed = 0;
  size_t scannedFramesAcquired = 0;
  aLUL->Unwind( &framePCs[0], &frameSPs[0],
                &framesUsed, &scannedFramesAcquired,
                framesAvail, scannedFramesAllowed,
                &startRegs, stackImg );

  delete stackImg;

  
  
  
  
  
  
  
  
  

  
  
  
  uintptr_t binding[8];  
  memset((void*)binding, 0, sizeof(binding));

  
  
  
  const char* cursor = dstring;

  
  
  while (*cursor) cursor++;

  
  size_t nConsistent = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  size_t frameIx;
  for (cursor = cursor-2, frameIx = 2;
       cursor >= dstring && frameIx < framesUsed;
       cursor--, frameIx++) {
    char      c  = *cursor;
    uintptr_t pc = framePCs[frameIx];
    
    MOZ_ASSERT(c >= '1' && c <= '8');
    int n = ((int)c) - ((int)'1');
    if (binding[n] == 0) {
      
      binding[n] = pc;
      nConsistent++;
      continue;
    }
    
    if (binding[n] != pc) {
      
      break;
    }
    
    nConsistent++;
  }

  
  bool passed = nConsistent+1 == strlen(dstring);

  
  char buf[200];
  snprintf(buf, sizeof(buf), "LULUnitTest:   dstring = %s\n", dstring);
  buf[sizeof(buf)-1] = 0;
  aLUL->mLog(buf);
  snprintf(buf, sizeof(buf),
           "LULUnitTest:     %d consistent, %d in dstring: %s\n",
           (int)nConsistent, (int)strlen(dstring),
           passed ? "PASS" : "FAIL");
  buf[sizeof(buf)-1] = 0;
  aLUL->mLog(buf);

  return passed;
}










#define DECL_TEST_FN(NAME) \
  bool NAME(LUL* aLUL, const char* strPorig, const char* strP);

#define GEN_TEST_FN(NAME, FRAMESIZE) \
  bool NAME(LUL* aLUL, const char* strPorig, const char* strP) { \
    volatile char space[FRAMESIZE]; \
    memset((char*)&space[0], 0, sizeof(space)); \
    if (*strP == '\0') { \
      /* We've come to the end of the director string. */ \
      /* Take a stack snapshot. */ \
      return GetAndCheckStackTrace(aLUL, strPorig); \
    } else { \
      /* Recurse onwards.  This is a bit subtle.  The obvious */ \
      /* thing to do here is call onwards directly, from within the */ \
      /* arms of the case statement.  That gives a problem in that */ \
      /* there will be multiple return points inside each function when */ \
      /* unwinding, so it will be difficult to check for consistency */ \
      /* against the director string.  Instead, we make an indirect */ \
      /* call, so as to guarantee that there is only one call site */ \
      /* within each function.  This does assume that the compiler */ \
      /* won't transform it back to the simple direct-call form. */ \
      /* To discourage it from doing so, the call is bracketed with */ \
      /* __asm__ __volatile__ sections so as to make it not-movable. */ \
      bool (*nextFn)(LUL*, const char*, const char*) = NULL; \
      switch (*strP) { \
        case '1': nextFn = TestFn1; break; \
        case '2': nextFn = TestFn2; break; \
        case '3': nextFn = TestFn3; break; \
        case '4': nextFn = TestFn4; break; \
        case '5': nextFn = TestFn5; break; \
        case '6': nextFn = TestFn6; break; \
        case '7': nextFn = TestFn7; break; \
        case '8': nextFn = TestFn8; break; \
        default:  nextFn = TestFn8; break; \
      } \
      __asm__ __volatile__("":::"cc","memory"); \
      bool passed = nextFn(aLUL, strPorig, strP+1); \
      __asm__ __volatile__("":::"cc","memory"); \
      return passed; \
    } \
  }



DECL_TEST_FN(TestFn1)
DECL_TEST_FN(TestFn2)
DECL_TEST_FN(TestFn3)
DECL_TEST_FN(TestFn4)
DECL_TEST_FN(TestFn5)
DECL_TEST_FN(TestFn6)
DECL_TEST_FN(TestFn7)
DECL_TEST_FN(TestFn8)

GEN_TEST_FN(TestFn1, 123)
GEN_TEST_FN(TestFn2, 456)
GEN_TEST_FN(TestFn3, 789)
GEN_TEST_FN(TestFn4, 23)
GEN_TEST_FN(TestFn5, 47)
GEN_TEST_FN(TestFn6, 117)
GEN_TEST_FN(TestFn7, 1)
GEN_TEST_FN(TestFn8, 99)










__attribute__((noinline)) void
TestUnw(int* aNTests, int*aNTestsPassed,
        LUL* aLUL, const char* dstring)
{
  
  
  
  
  
  
  
  
  
  int i;
  volatile char space[LUL_UNIT_TEST_STACK_SIZE];
  for (i = 0; i < LUL_UNIT_TEST_STACK_SIZE; i++) {
    space[i] = (char)(i & 0x7F);
  }

  
  bool passed = TestFn1(aLUL, dstring, dstring);

  
  
  int sum = 0;
  for (i = 0; i < LUL_UNIT_TEST_STACK_SIZE; i++) {
    
    sum += space[i] - 3*i;
  }
  __asm__ __volatile__("" : : "r"(sum));

  
  (*aNTests)++;
  if (passed) {
    (*aNTestsPassed)++;
  }
}


void
RunLulUnitTests(int* aNTests, int*aNTestsPassed, LUL* aLUL)
{
  aLUL->mLog(":\n");
  aLUL->mLog("LULUnitTest: BEGIN\n");
  *aNTests = *aNTestsPassed = 0;
  TestUnw(aNTests, aNTestsPassed, aLUL, "11111111");
  TestUnw(aNTests, aNTestsPassed, aLUL, "11222211");
  TestUnw(aNTests, aNTestsPassed, aLUL, "111222333");
  TestUnw(aNTests, aNTestsPassed, aLUL, "1212121231212331212121212121212");
  TestUnw(aNTests, aNTestsPassed, aLUL, "31415827271828325332173258");
  TestUnw(aNTests, aNTestsPassed, aLUL,
          "123456781122334455667788777777777777777777777");
  aLUL->mLog("LULUnitTest: END\n");
  aLUL->mLog(":\n");
}


} 
