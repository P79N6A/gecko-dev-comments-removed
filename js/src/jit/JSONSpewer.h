





#ifndef jit_JSONSpewer_h
#define jit_JSONSpewer_h

#include <stdio.h>

#include "js/TypeDecls.h"

#include "vm/Printer.h"

namespace js {
namespace jit {

class BacktrackingAllocator;
class MDefinition;
class MIRGraph;
class MResumePoint;
class LNode;

class JSONSpewer
{
  private:
    
    
    bool inFunction_;

    int indentLevel_;
    bool first_;
    Fprinter out_;

    void indent();

    void property(const char* name);
    void beginObject();
    void beginObjectProperty(const char* name);
    void beginListProperty(const char* name);
    void stringValue(const char* format, ...);
    void stringProperty(const char* name, const char* format, ...);
    void integerValue(int value);
    void integerProperty(const char* name, int value);
    void endObject();
    void endList();

  public:
    JSONSpewer()
      : inFunction_(false),
        indentLevel_(0),
        first_(true),
        out_()
    { }
    ~JSONSpewer();

    bool init(const char* path);
    void beginFunction(JSScript* script);
    void beginPass(const char * pass);
    void spewMDef(MDefinition* def);
    void spewMResumePoint(MResumePoint* rp);
    void spewMIR(MIRGraph* mir);
    void spewLIns(LNode* ins);
    void spewLIR(MIRGraph* mir);
    void spewRanges(BacktrackingAllocator* regalloc);
    void endPass();
    void endFunction();
    void finish();
};

} 
} 

#endif 
