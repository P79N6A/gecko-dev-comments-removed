























#include "EHABIStackWalk.h"

#include "shared-libraries.h"
#include "platform.h"

#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Endian.h"

#include <algorithm>
#include <elf.h>
#include <stdint.h>
#include <vector>
#include <string>

#ifndef PT_ARM_EXIDX
#define PT_ARM_EXIDX 0x70000001
#endif






#if defined(ANDROID_VERSION) && ANDROID_VERSION < 16
#define HAVE_UNSORTED_EXIDX
#endif

namespace mozilla {

struct PRel31 {
  uint32_t mBits;
  bool topBit() const { return mBits & 0x80000000; }
  uint32_t value() const { return mBits & 0x7fffffff; }
  int32_t offset() const { return (static_cast<int32_t>(mBits) << 1) >> 1; }
  const void *compute() const {
    return reinterpret_cast<const char *>(this) + offset();
  }
private:
  PRel31(const PRel31 &copied) = delete;
  PRel31() = delete;
};

struct EHEntry {
  PRel31 startPC;
  PRel31 exidx;
private:
  EHEntry(const EHEntry &copied) = delete;
  EHEntry() = delete;
};

class EHState {
  
  
  uint32_t mRegs[16];
public:
  bool unwind(const EHEntry *aEntry, const void *stackBase);
  uint32_t &operator[](int i) { return mRegs[i]; }
  const uint32_t &operator[](int i) const { return mRegs[i]; }
  EHState(const mcontext_t &);
};

enum {
  R_SP = 13,
  R_LR = 14,
  R_PC = 15
};

#ifdef HAVE_UNSORTED_EXIDX
class EHEntryHandle {
  const EHEntry *mValue;
public:
  EHEntryHandle(const EHEntry *aEntry) : mValue(aEntry) { }
  const EHEntry *value() const { return mValue; }
};

bool operator<(const EHEntryHandle &lhs, const EHEntryHandle &rhs) {
  return lhs.value()->startPC.compute() < rhs.value()->startPC.compute();
}
#endif

class EHTable {
  uint32_t mStartPC;
  uint32_t mEndPC;
  uint32_t mLoadOffset;
#ifdef HAVE_UNSORTED_EXIDX
  
  
  
  std::vector<EHEntryHandle> mEntries;
  typedef std::vector<EHEntryHandle>::const_iterator EntryIterator;
  EntryIterator entriesBegin() const { return mEntries.begin(); }
  EntryIterator entriesEnd() const { return mEntries.end(); }
  static const EHEntry* entryGet(EntryIterator aEntry) {
    return aEntry->value();
  }
#else
  typedef const EHEntry *EntryIterator;
  EntryIterator mEntriesBegin, mEntriesEnd;
  EntryIterator entriesBegin() const { return mEntriesBegin; }
  EntryIterator entriesEnd() const { return mEntriesEnd; }
  static const EHEntry* entryGet(EntryIterator aEntry) { return aEntry; }
#endif
  std::string mName;
public:
  EHTable(const void *aELF, size_t aSize, const std::string &aName);
  const EHEntry *lookup(uint32_t aPC) const;
  bool isValid() const { return entriesEnd() != entriesBegin(); }
  const std::string &name() const { return mName; }
  uint32_t startPC() const { return mStartPC; }
  uint32_t endPC() const { return mEndPC; }
  uint32_t loadOffset() const { return mLoadOffset; }
};

class EHAddrSpace {
  std::vector<uint32_t> mStarts;
  std::vector<EHTable> mTables;
  static mozilla::Atomic<const EHAddrSpace*> sCurrent;
public:
  explicit EHAddrSpace(const std::vector<EHTable>& aTables);
  const EHTable *lookup(uint32_t aPC) const;
  static void Update();
  static const EHAddrSpace *Get();
};


void EHABIStackWalkInit()
{
  EHAddrSpace::Update();
}

size_t EHABIStackWalk(const mcontext_t &aContext, void *stackBase,
                      void **aSPs, void **aPCs, const size_t aNumFrames)
{
  const EHAddrSpace *space = EHAddrSpace::Get();
  EHState state(aContext);
  size_t count = 0;

  while (count < aNumFrames) {
    uint32_t pc = state[R_PC], sp = state[R_SP];
    aPCs[count] = reinterpret_cast<void *>(pc);
    aSPs[count] = reinterpret_cast<void *>(sp);
    count++;

    if (!space)
      break;
    
    
    
    const EHTable *table = space->lookup(pc);
    if (!table)
      break;
    const EHEntry *entry = table->lookup(pc);
    if (!entry)
      break;
    if (!state.unwind(entry, stackBase))
      break;
  }
  
  return count;
}


class EHInterp {
public:
  
  
  
  EHInterp(EHState &aState, const EHEntry *aEntry,
           uint32_t aStackLimit, uint32_t aStackBase)
    : mState(aState),
      mStackLimit(aStackLimit),
      mStackBase(aStackBase),
      mNextWord(0),
      mWordsLeft(0),
      mFailed(false)
  {
    const PRel31 &exidx = aEntry->exidx;
    uint32_t firstWord;

    if (exidx.mBits == 1) {  
      mFailed = true;
      return;
    }
    if (exidx.topBit()) {
      firstWord = exidx.mBits;
    } else {
      mNextWord = reinterpret_cast<const uint32_t *>(exidx.compute());
      firstWord = *mNextWord++;
    }

    switch (firstWord >> 24) {
    case 0x80: 
      mWord = firstWord << 8;
      mBytesLeft = 3;
      break;
    case 0x81: case 0x82: 
      mWord = firstWord << 16;
      mBytesLeft = 2;
      mWordsLeft = (firstWord >> 16) & 0xff;
      break;
    default:
      
      mFailed = true;
    }
  }

  bool unwind();

private:
  
  
  
  
  
  EHState &mState;
  uint32_t mStackLimit;
  uint32_t mStackBase;
  const uint32_t *mNextWord;
  uint32_t mWord;
  uint8_t mWordsLeft;
  uint8_t mBytesLeft;
  bool mFailed;

  enum {
    I_ADDSP    = 0x00, 
    M_ADDSP    = 0x80,
    I_POPMASK  = 0x80, 
    M_POPMASK  = 0xf0,
    I_MOVSP    = 0x90, 
    M_MOVSP    = 0xf0,
    I_POPN     = 0xa0, 
    M_POPN     = 0xf0,
    I_FINISH   = 0xb0, 
    I_POPLO    = 0xb1, 
    I_ADDSPBIG = 0xb2, 
    I_POPFDX   = 0xb3, 
    I_POPFDX8  = 0xb8, 
    M_POPFDX8  = 0xf8,
    
    I_POPFDD   = 0xc8, 
    M_POPFDD   = 0xfe,
    I_POPFDD8  = 0xd0, 
    M_POPFDD8  = 0xf8
  };

  uint8_t next() {
    if (mBytesLeft == 0) {
      if (mWordsLeft == 0) {
        return I_FINISH;
      }
      mWordsLeft--;
      mWord = *mNextWord++;
      mBytesLeft = 4;
    }
    mBytesLeft--;
    mWord = (mWord << 8) | (mWord >> 24); 
    return mWord;
  }

  uint32_t &vSP() { return mState[R_SP]; }
  uint32_t *ptrSP() { return reinterpret_cast<uint32_t *>(vSP()); }

  void checkStackBase() { if (vSP() > mStackBase) mFailed = true; }
  void checkStackLimit() { if (vSP() <= mStackLimit) mFailed = true; }
  void checkStackAlign() { if ((vSP() & 3) != 0) mFailed = true; }
  void checkStack() {
    checkStackBase();
    checkStackLimit();
    checkStackAlign();
  }

  void popRange(uint8_t first, uint8_t last, uint16_t mask) {
    bool hasSP = false;
    uint32_t tmpSP;
    if (mask == 0)
      mFailed = true;
    for (uint8_t r = first; r <= last; ++r) {
      if (mask & 1) {
        if (r == R_SP) {
          hasSP = true;
          tmpSP = *ptrSP();
        } else
          mState[r] = *ptrSP();
        vSP() += 4;
        checkStackBase();
        if (mFailed)
          return;
      }
      mask >>= 1;
    }
    if (hasSP) {
      vSP() = tmpSP;
      checkStack();
    }
  }
};


bool EHState::unwind(const EHEntry *aEntry, const void *stackBasePtr) {
  
  uint32_t stackLimit = mRegs[R_SP] - 4;
  uint32_t stackBase = reinterpret_cast<uint32_t>(stackBasePtr);
  EHInterp interp(*this, aEntry, stackLimit, stackBase);
  return interp.unwind();
}

bool EHInterp::unwind() {
  mState[R_PC] = 0;
  checkStack();
  while (!mFailed) {
    uint8_t insn = next();
#if DEBUG_EHABI_UNWIND
    LOGF("unwind insn = %02x", (unsigned)insn);
#endif
    

    
    
    if ((insn & M_ADDSP) == I_ADDSP) {
      uint32_t offset = ((insn & 0x3f) << 2) + 4;
      if (insn & 0x40) {
        vSP() -= offset;
        checkStackLimit();
      } else {
        vSP() += offset;
        checkStackBase();
      }
      continue;
    }

    
    
    if ((insn & M_POPN) == I_POPN) {
      uint8_t n = (insn & 0x07) + 1;
      bool lr = insn & 0x08;
      uint32_t *ptr = ptrSP();
      vSP() += (n + (lr ? 1 : 0)) * 4;
      checkStackBase();
      for (uint8_t r = 4; r < 4 + n; ++r)
        mState[r] = *ptr++;
      if (lr)
        mState[R_LR] = *ptr++;
      continue;
    }

    
    if (insn == I_FINISH) {
      if (mState[R_PC] == 0) {
        mState[R_PC] = mState[R_LR];
        
        
        
        
        
        
        
        
        
        mState[R_LR] = 0;
      }
      return true;
    }

    
    if ((insn & M_MOVSP) == I_MOVSP) {
      vSP() = mState[insn & 0x0f];
      checkStack();
      continue;
    }

    
    
    if ((insn & M_POPFDD) == I_POPFDD) {
      uint8_t n = (next() & 0x0f) + 1;
      
      
      vSP() += 8 * n;
      checkStackBase();
      continue;
    }

    
    if ((insn & M_POPFDD8) == I_POPFDD8) {
      uint8_t n = (insn & 0x07) + 1;
      vSP() += 8 * n;
      checkStackBase();
      continue;
    }

    
    if (insn == I_ADDSPBIG) {
      uint32_t acc = 0;
      uint8_t shift = 0;
      uint8_t byte;
      do {
        if (shift >= 32)
          return false;
        byte = next();
        acc |= (byte & 0x7f) << shift;
        shift += 7;
      } while (byte & 0x80);
      uint32_t offset = 0x204 + (acc << 2);
      
      
      if (vSP() + offset < vSP())
        mFailed = true;
      vSP() += offset;
      
      checkStackBase();
      continue;
    }

    
    if ((insn & M_POPMASK) == I_POPMASK) {
      popRange(4, 15, ((insn & 0x0f) << 8) | next());
      continue;
    }

    
    if (insn == I_POPLO) {
      popRange(0, 3, next() & 0x0f);
      continue;
    }

    
    if (insn == I_POPFDX) {
      uint8_t n = (next() & 0x0f) + 1;
      vSP() += 8 * n + 4;
      checkStackBase();
      continue;
    }

    
    if ((insn & M_POPFDX8) == I_POPFDX8) {
      uint8_t n = (insn & 0x07) + 1;
      vSP() += 8 * n + 4;
      checkStackBase();
      continue;
    }

    
#ifdef DEBUG_EHABI_UNWIND
    LOGF("Unhandled EHABI instruction 0x%02x", insn);
#endif
    mFailed = true;
  }
  return false;
}


bool operator<(const EHTable &lhs, const EHTable &rhs) {
  return lhs.startPC() < rhs.endPC();
}


EHAddrSpace::EHAddrSpace(const std::vector<EHTable>& aTables)
  : mTables(aTables)
{
  std::sort(mTables.begin(), mTables.end());
  DebugOnly<uint32_t> lastEnd = 0;
  for (std::vector<EHTable>::iterator i = mTables.begin();
       i != mTables.end(); ++i) {
    MOZ_ASSERT(i->startPC() >= lastEnd);
    mStarts.push_back(i->startPC());
    lastEnd = i->endPC();
  }
}

const EHTable *EHAddrSpace::lookup(uint32_t aPC) const {
  ptrdiff_t i = (std::upper_bound(mStarts.begin(), mStarts.end(), aPC)
                 - mStarts.begin()) - 1;

  if (i < 0 || aPC >= mTables[i].endPC())
    return 0;
  return &mTables[i];
}


const EHEntry *EHTable::lookup(uint32_t aPC) const {
  MOZ_ASSERT(aPC >= mStartPC);
  if (aPC >= mEndPC)
    return nullptr;

  EntryIterator begin = entriesBegin();
  EntryIterator end = entriesEnd();
  MOZ_ASSERT(begin < end);
  if (aPC < reinterpret_cast<uint32_t>(entryGet(begin)->startPC.compute()))
    return nullptr;

  while (end - begin > 1) {
#ifdef EHABI_UNWIND_MORE_ASSERTS
    if (entryGet(end - 1)->startPC.compute()
        < entryGet(begin)->startPC.compute()) {
      MOZ_CRASH("unsorted exidx");
    }
#endif
    EntryIterator mid = begin + (end - begin) / 2;
    if (aPC < reinterpret_cast<uint32_t>(entryGet(mid)->startPC.compute()))
      end = mid;
    else
      begin = mid;
  }
  return entryGet(begin);
}


#if MOZ_LITTLE_ENDIAN
static const unsigned char hostEndian = ELFDATA2LSB;
#elif MOZ_BIG_ENDIAN
static const unsigned char hostEndian = ELFDATA2MSB;
#else
#error "No endian?"
#endif


EHTable::EHTable(const void *aELF, size_t aSize, const std::string &aName)
  : mStartPC(~0), 
    mEndPC(0),
#ifndef HAVE_UNSORTED_EXIDX
    mEntriesBegin(nullptr),
    mEntriesEnd(nullptr),
#endif
    mName(aName)
{
  const uint32_t base = reinterpret_cast<uint32_t>(aELF);

  if (aSize < sizeof(Elf32_Ehdr))
    return;

  const Elf32_Ehdr &file = *(reinterpret_cast<Elf32_Ehdr *>(base));
  if (memcmp(&file.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
      file.e_ident[EI_CLASS] != ELFCLASS32 ||
      file.e_ident[EI_DATA] != hostEndian ||
      file.e_ident[EI_VERSION] != EV_CURRENT ||
      file.e_ident[EI_OSABI] != ELFOSABI_SYSV ||
#ifdef EI_ABIVERSION
      file.e_ident[EI_ABIVERSION] != 0 ||
#endif
      file.e_machine != EM_ARM ||
      file.e_version != EV_CURRENT)
    
    return;

  MOZ_ASSERT(file.e_phoff + file.e_phnum * file.e_phentsize <= aSize);
  const Elf32_Phdr *exidxHdr = 0, *zeroHdr = 0;
  for (unsigned i = 0; i < file.e_phnum; ++i) {
    const Elf32_Phdr &phdr =
      *(reinterpret_cast<Elf32_Phdr *>(base + file.e_phoff
                                       + i * file.e_phentsize));
    if (phdr.p_type == PT_ARM_EXIDX) {
      exidxHdr = &phdr;
    } else if (phdr.p_type == PT_LOAD) {
      if (phdr.p_offset == 0) {
        zeroHdr = &phdr;
      }
      if (phdr.p_flags & PF_X) {
        mStartPC = std::min(mStartPC, phdr.p_vaddr);
        mEndPC = std::max(mEndPC, phdr.p_vaddr + phdr.p_memsz);
      }
    }
  }
  if (!exidxHdr)
    return;
  if (!zeroHdr)
    return;
  mLoadOffset = base - zeroHdr->p_vaddr;
  mStartPC += mLoadOffset;
  mEndPC += mLoadOffset;

  
  const EHEntry *startTable =
    reinterpret_cast<const EHEntry *>(mLoadOffset + exidxHdr->p_vaddr);
  const EHEntry *endTable =
    reinterpret_cast<const EHEntry *>(mLoadOffset + exidxHdr->p_vaddr
                                    + exidxHdr->p_memsz);
#ifdef HAVE_UNSORTED_EXIDX
  mEntries.reserve(endTable - startTable);
  for (const EHEntry *i = startTable; i < endTable; ++i)
    mEntries.push_back(i);
  std::sort(mEntries.begin(), mEntries.end());
#else
  mEntriesBegin = startTable;
  mEntriesEnd = endTable;
#endif
}


mozilla::Atomic<const EHAddrSpace*> EHAddrSpace::sCurrent(nullptr);


const EHAddrSpace *EHAddrSpace::Get() {
  return sCurrent;
}



void EHAddrSpace::Update() {
  const EHAddrSpace *space = sCurrent;
  if (space)
    return;

  SharedLibraryInfo info = SharedLibraryInfo::GetInfoForSelf();
  std::vector<EHTable> tables;

  for (size_t i = 0; i < info.GetSize(); ++i) {
    const SharedLibrary &lib = info.GetEntry(i);
    if (lib.GetOffset() != 0)
      
      
      
      
      
      continue;
    EHTable tab(reinterpret_cast<const void *>(lib.GetStart()),
              lib.GetEnd() - lib.GetStart(), lib.GetName());
    if (tab.isValid())
      tables.push_back(tab);
  }
  space = new EHAddrSpace(tables);

  if (!sCurrent.compareExchange(nullptr, space)) {
    delete space;
    space = sCurrent;
  }
}


EHState::EHState(const mcontext_t &context) {
#ifdef linux
  mRegs[0] = context.arm_r0;
  mRegs[1] = context.arm_r1;
  mRegs[2] = context.arm_r2;
  mRegs[3] = context.arm_r3;
  mRegs[4] = context.arm_r4;
  mRegs[5] = context.arm_r5;
  mRegs[6] = context.arm_r6;
  mRegs[7] = context.arm_r7;
  mRegs[8] = context.arm_r8;
  mRegs[9] = context.arm_r9;
  mRegs[10] = context.arm_r10;
  mRegs[11] = context.arm_fp;
  mRegs[12] = context.arm_ip;
  mRegs[13] = context.arm_sp;
  mRegs[14] = context.arm_lr;
  mRegs[15] = context.arm_pc;
#else
# error "Unhandled OS for ARM EHABI unwinding"
#endif
}

} 

