






#ifndef js_ion_jsonspewer_h__
#define js_ion_jsonspewer_h__

#include <stdio.h>

#include "gc/Root.h"
#include "jsscript.h"

class JSScript;

namespace js {
namespace ion {

class MDefinition;
class MInstruction;
class MBasicBlock;
class MIRGraph;
class MResumePoint;
class LinearScanAllocator;
class LInstruction;

class JSONSpewer
{
  private:
    
    
    bool inFunction_;

    int indentLevel_;
    bool first_;
    FILE *fp_;

    void indent();

    void property(const char *name);
    void beginObject();
    void beginObjectProperty(const char *name);
    void beginListProperty(const char *name);
    void stringValue(const char *format, ...);
    void stringProperty(const char *name, const char *format, ...);
    void integerValue(int value);
    void integerProperty(const char *name, int value);
    void endObject();
    void endList();

  public:
    JSONSpewer()
      : inFunction_(false),
        indentLevel_(0),
        first_(true),
        fp_(NULL)
    { }
    ~JSONSpewer();

    bool init(const char *path);
    void beginFunction(RawScript script);
    void beginPass(const char * pass);
    void spewMDef(MDefinition *def);
    void spewMResumePoint(MResumePoint *rp);
    void spewMIR(MIRGraph *mir);
    void spewLIns(LInstruction *ins);
    void spewLIR(MIRGraph *mir);
    void spewIntervals(LinearScanAllocator *regalloc);
    void endPass();
    void endFunction();
    void finish();
};

} 
} 

#endif 

