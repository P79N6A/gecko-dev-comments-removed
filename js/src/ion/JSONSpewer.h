








































#ifndef js_ion_jsonspewer_h__
#define js_ion_jsonspewer_h__

#include <stdio.h>
#include "MIR.h"
#include "MIRGraph.h"
#include "IonLIR.h"
#include "LinearScan.h"

namespace js {
namespace ion {

class JSONSpewer
{
  private:
    bool first_;
    FILE *fp_;

    void property(const char *name);
    void beginObject();
    void beginObjectProperty(const char *name);
    void beginListProperty(const char *name);
    void stringProperty(const char *name, const char *format, ...);
    void integerValue(int value);
    void integerProperty(const char *name, int value);
    void endObject();
    void endList();

  public:
    JSONSpewer()
      : first_(true)
    { }

    bool init(const char *path);
    void beginFunction(JSScript *script);
    void beginPass(const char * pass);
    void spewMIR(MIRGraph *mir);
    void spewLIR(MIRGraph *mir);
    void spewIntervals(LinearScanAllocator *regalloc);
    void endPass();
    void endFunction();
    void finish();
};

}
}

#endif
