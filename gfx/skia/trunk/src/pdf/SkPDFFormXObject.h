








#ifndef SkPDFFormXObject_DEFINED
#define SkPDFFormXObject_DEFINED

#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkPDFResourceDict.h"
#include "SkString.h"

class SkMatrix;
class SkPDFDevice;
class SkPDFCatalog;











class SkPDFFormXObject : public SkPDFStream {
public:
    



    explicit SkPDFFormXObject(SkPDFDevice* device);
    



    explicit SkPDFFormXObject(SkStream* content,
                              SkRect bbox,
                              SkPDFResourceDict* resourceDict);
    virtual ~SkPDFFormXObject();

    
    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

private:
    void init(const char* colorSpace,
              SkPDFDict* resourceDict, SkPDFArray* bbox);

    SkTSet<SkPDFObject*> fResources;
};

#endif
