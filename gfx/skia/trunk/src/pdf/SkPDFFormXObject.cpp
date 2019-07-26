








#include "SkPDFFormXObject.h"

#include "SkMatrix.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFResourceDict.h"
#include "SkPDFUtils.h"
#include "SkStream.h"
#include "SkTypes.h"

SkPDFFormXObject::SkPDFFormXObject(SkPDFDevice* device) {
    
    
    
    SkTSet<SkPDFObject*> emptySet;
    SkPDFResourceDict* resourceDict = device->getResourceDict();
    resourceDict->getReferencedResources(emptySet, &fResources, false);

    SkAutoTUnref<SkStream> content(device->content());
    setData(content.get());

    SkAutoTUnref<SkPDFArray> bboxArray(device->copyMediaBox());
    init(NULL, resourceDict, bboxArray);

    
    
    
    if (!device->initialTransform().isIdentity()) {
        SkMatrix inverse;
        if (!device->initialTransform().invert(&inverse)) {
            
            SkASSERT(false);
            inverse.reset();
        }
        insert("Matrix", SkPDFUtils::MatrixToArray(inverse))->unref();
    }
}




SkPDFFormXObject::SkPDFFormXObject(SkStream* content, SkRect bbox,
                                   SkPDFResourceDict* resourceDict) {
    SkTSet<SkPDFObject*> emptySet;
    resourceDict->getReferencedResources(emptySet, &fResources, false);

    setData(content);

    SkAutoTUnref<SkPDFArray> bboxArray(SkPDFUtils::RectToArray(bbox));
    init("DeviceRGB", resourceDict, bboxArray);
}





void SkPDFFormXObject::init(const char* colorSpace,
                            SkPDFDict* resourceDict, SkPDFArray* bbox) {
    insertName("Type", "XObject");
    insertName("Subtype", "Form");
    insert("Resources", resourceDict);
    insert("BBox", bbox);

    
    
    SkAutoTUnref<SkPDFDict> group(new SkPDFDict("Group"));
    group->insertName("S", "Transparency");

    if (colorSpace != NULL) {
        group->insertName("CS", colorSpace);
    }
    group->insert("I", new SkPDFBool(true))->unref();  
    insert("Group", group.get());
}

SkPDFFormXObject::~SkPDFFormXObject() {
    fResources.unrefAll();
}

void SkPDFFormXObject::getResources(
        const SkTSet<SkPDFObject*>& knownResourceObjects,
        SkTSet<SkPDFObject*>* newResourceObjects) {
    GetResourcesHelper(&fResources.toArray(),
                       knownResourceObjects,
                       newResourceObjects);
}
