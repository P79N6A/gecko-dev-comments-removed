






#include "SkFlattenable.h"
#include "SkPtrRecorder.h"

SK_DEFINE_INST_COUNT(SkFlattenable)



void SkFlattenable::flatten(SkFlattenableWriteBuffer&) const
{
    



}



SkNamedFactorySet::SkNamedFactorySet() : fNextAddedFactory(0) {}

uint32_t SkNamedFactorySet::find(SkFlattenable::Factory factory) {
    uint32_t index = fFactorySet.find(factory);
    if (index > 0) {
        return index;
    }
    const char* name = SkFlattenable::FactoryToName(factory);
    if (NULL == name) {
        return 0;
    }
    *fNames.append() = name;
    return fFactorySet.add(factory);
}

const char* SkNamedFactorySet::getNextAddedFactoryName() {
    if (fNextAddedFactory < fNames.count()) {
        return fNames[fNextAddedFactory++];
    }
    return NULL;
}



SkRefCntSet::~SkRefCntSet() {
    
    this->reset();
}

void SkRefCntSet::incPtr(void* ptr) {
    ((SkRefCnt*)ptr)->ref();
}

void SkRefCntSet::decPtr(void* ptr) {
    ((SkRefCnt*)ptr)->unref();
}





#define MAX_PAIR_COUNT  1024

struct Pair {
    const char*             fName;
    SkFlattenable::Factory  fFactory;
};

static int gCount;
static Pair gPairs[MAX_PAIR_COUNT];

void SkFlattenable::Register(const char name[], Factory factory) {
    SkASSERT(name);
    SkASSERT(factory);

    static bool gOnce;
    if (!gOnce) {
        gCount = 0;
        gOnce = true;
    }

    SkASSERT(gCount < MAX_PAIR_COUNT);

    gPairs[gCount].fName = name;
    gPairs[gCount].fFactory = factory;
    gCount += 1;
}

#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS && defined(SK_DEBUG)
static void report_no_entries(const char* functionName) {
    if (!gCount) {
        SkDebugf("%s has no registered name/factory pairs."
                 " Call SkGraphics::Init() at process initialization time.",
                 functionName);
    }
}
#endif

SkFlattenable::Factory SkFlattenable::NameToFactory(const char name[]) {
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS && defined(SK_DEBUG)
    report_no_entries(__FUNCTION__);
#endif
    const Pair* pairs = gPairs;
    for (int i = gCount - 1; i >= 0; --i) {
        if (strcmp(pairs[i].fName, name) == 0) {
            return pairs[i].fFactory;
        }
    }
    return NULL;
}

const char* SkFlattenable::FactoryToName(Factory fact) {
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS && defined(SK_DEBUG)
    report_no_entries(__FUNCTION__);
#endif
    const Pair* pairs = gPairs;
    for (int i = gCount - 1; i >= 0; --i) {
        if (pairs[i].fFactory == fact) {
            return pairs[i].fName;
        }
    }
    return NULL;
}
