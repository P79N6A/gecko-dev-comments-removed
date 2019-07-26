








#include "SkPDFFormXObject.h"

#include "SkMatrix.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFUtils.h"
#include "SkStream.h"
#include "SkTypes.h"

SkPDFFormXObject::SkPDFFormXObject(SkPDFDevice* device) {
    
    
    
    SkTSet<SkPDFObject*> emptySet;
    device->getResources(emptySet, &fResources, false);

    SkAutoTUnref<SkStream> content(device->content());
    setData(content.get());

    insertName("Type", "XObject");
    insertName("Subtype", "Form");
    SkSafeUnref(this->insert("BBox", device->copyMediaBox()));
    insert("Resources", device->getResourceDict());

    
    
    
    if (!device->initialTransform().isIdentity()) {
        SkMatrix inverse;
        if (!device->initialTransform().invert(&inverse)) {
            
            SkASSERT(false);
            inverse.reset();
        }
        insert("Matrix", SkPDFUtils::MatrixToArray(inverse))->unref();
    }

    
    
    SkAutoTUnref<SkPDFDict> group(new SkPDFDict("Group"));
    group->insertName("S", "Transparency");
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
