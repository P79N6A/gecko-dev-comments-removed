








#include "SkPDFFormXObject.h"

#include "SkMatrix.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFUtils.h"
#include "SkStream.h"
#include "SkTypes.h"

SkPDFFormXObject::SkPDFFormXObject(SkPDFDevice* device) {
    
    
    
    device->getResources(&fResources);

    SkRefPtr<SkStream> content = device->content();
    content->unref();  
    setData(content.get());

    insertName("Type", "XObject");
    insertName("Subtype", "Form");
    insert("BBox", device->getMediaBox().get());
    insert("Resources", device->getResourceDict());

    
    
    
    if (!device->initialTransform().isIdentity()) {
        SkMatrix inverse;
        inverse.reset();
        device->initialTransform().invert(&inverse);
        insert("Matrix", SkPDFUtils::MatrixToArray(inverse))->unref();
    }

    
    
    SkRefPtr<SkPDFDict> group = new SkPDFDict("Group");
    group->unref();  
    group->insertName("S", "Transparency");
    group->insert("I", new SkPDFBool(true))->unref();  
    insert("Group", group.get());
}

SkPDFFormXObject::~SkPDFFormXObject() {
    fResources.unrefAll();
}

void SkPDFFormXObject::getResources(SkTDArray<SkPDFObject*>* resourceList) {
    GetResourcesHelper(&fResources, resourceList);
}
