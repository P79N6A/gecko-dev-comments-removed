








#ifndef SkPDFFormXObject_DEFINED
#define SkPDFFormXObject_DEFINED

#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkRefCnt.h"
#include "SkString.h"

class SkMatrix;
class SkPDFDevice;
class SkPDFCatalog;











class SkPDFFormXObject : public SkPDFStream {
public:
    



    explicit SkPDFFormXObject(SkPDFDevice* device);
    virtual ~SkPDFFormXObject();

    
    virtual void getResources(SkTDArray<SkPDFObject*>* resourceList);

private:
    SkTDArray<SkPDFObject*> fResources;
};

#endif
