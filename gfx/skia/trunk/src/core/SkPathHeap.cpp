






#include "SkPathHeap.h"
#include "SkPath.h"
#include "SkStream.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include <new>

#define kPathCount  64

SkPathHeap::SkPathHeap() : fHeap(kPathCount * sizeof(SkPath)) {
}

SkPathHeap::SkPathHeap(SkReadBuffer& buffer)
            : fHeap(kPathCount * sizeof(SkPath)) {
    const int count = buffer.readInt();

    fPaths.setCount(count);
    SkPath** ptr = fPaths.begin();
    SkPath* p = (SkPath*)fHeap.allocThrow(count * sizeof(SkPath));

    for (int i = 0; i < count; i++) {
        new (p) SkPath;
        buffer.readPath(p);
        *ptr++ = p; 
        p++;        
    }
}

SkPathHeap::~SkPathHeap() {
    SkPath** iter = fPaths.begin();
    SkPath** stop = fPaths.end();
    while (iter < stop) {
        (*iter)->~SkPath();
        iter++;
    }
}

int SkPathHeap::append(const SkPath& path) {
    SkPath* p = (SkPath*)fHeap.allocThrow(sizeof(SkPath));
    new (p) SkPath(path);
    *fPaths.append() = p;
    return fPaths.count();
}

void SkPathHeap::flatten(SkWriteBuffer& buffer) const {
    int count = fPaths.count();

    buffer.writeInt(count);
    SkPath* const* iter = fPaths.begin();
    SkPath* const* stop = fPaths.end();
    while (iter < stop) {
        buffer.writePath(**iter);
        iter++;
    }
}
