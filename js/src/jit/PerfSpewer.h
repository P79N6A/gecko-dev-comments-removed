





#ifndef jit_PerfSpewer_h
#define jit_PerfSpewer_h

#include <stdio.h>

#include "jit/IonMacroAssembler.h"

namespace js {
namespace jit {

class MBasicBlock;
class MacroAssembler;

#ifdef JS_ION_PERF
void CheckPerf();
bool PerfBlockEnabled();
bool PerfFuncEnabled();
static inline bool PerfEnabled() {
    return PerfBlockEnabled() || PerfFuncEnabled();
}
#else
static inline void CheckPerf() {}
static inline bool PerfBlockEnabled() { return false; }
static inline bool PerfFuncEnabled() { return false; }
static inline bool PerfEnabled() { return false; }
#endif

#ifdef JS_ION_PERF

struct Record {
    const char *filename;
    unsigned lineNumber;
    unsigned columnNumber;
    uint32_t id;
    Label start, end;
    size_t startOffset, endOffset;

    Record(const char *filename,
           unsigned lineNumber,
           unsigned columnNumber,
           uint32_t id)
      : filename(filename), lineNumber(lineNumber),
        columnNumber(columnNumber), id(id),
        startOffset(0u), endOffset(0u)
    {}
};

typedef Vector<Record, 1, SystemAllocPolicy> BasicBlocksVector;

class PerfSpewer
{
  protected:
    static uint32_t nextFunctionIndex;

  public:
    Label endInlineCode;

  protected:
    BasicBlocksVector basicBlocks_;

  public:
    virtual bool startBasicBlock(MBasicBlock *blk, MacroAssembler &masm);
    bool endBasicBlock(MacroAssembler &masm);
    bool noteEndInlineCode(MacroAssembler &masm);

    void writeProfile(JSScript *script, IonCode *code, MacroAssembler &masm);
};

void writePerfSpewerBaselineProfile(JSScript *script, IonCode *code);
void writePerfSpewerIonCodeProfile(IonCode *code, const char *msg);

class AsmJSPerfSpewer : public PerfSpewer
{
  public:
    bool startBasicBlock(MBasicBlock *blk, MacroAssembler &masm);

    void noteBlocksOffsets();
    BasicBlocksVector &basicBlocks() { return basicBlocks_; }
};

void writePerfSpewerAsmJSFunctionMap(uintptr_t base, uintptr_t size, const char *filename,
                                     unsigned lineno, unsigned colIndex, const char *funcName);

void writePerfSpewerAsmJSBlocksMap(uintptr_t baseAddress, size_t funcStartOffset,
                                   size_t funcStartOOLOffset, size_t funcSize,
                                   const char *filename, const char *funcName,
                                   const BasicBlocksVector &basicBlocks);

void writePerfSpewerAsmJSEntriesAndExits(uintptr_t base, size_t size);

#endif 

} 
} 

#endif 
