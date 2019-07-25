








#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFPage.h"
#include "SkStream.h"

SkPDFPage::SkPDFPage(SkPDFDevice* content)
    : SkPDFDict("Page"),
      fDevice(content) {
}

SkPDFPage::~SkPDFPage() {}

void SkPDFPage::finalizePage(SkPDFCatalog* catalog, bool firstPage,
                             SkTDArray<SkPDFObject*>* resourceObjects) {
    if (fContentStream.get() == NULL) {
        insert("Resources", fDevice->getResourceDict());
        insert("MediaBox", fDevice->getMediaBox().get());

        SkRefPtr<SkStream> content = fDevice->content();
        content->unref();  
        fContentStream = new SkPDFStream(content.get());
        fContentStream->unref();  
        insert("Contents", new SkPDFObjRef(fContentStream.get()))->unref();
    }
    catalog->addObject(fContentStream.get(), firstPage);
    fDevice->getResources(resourceObjects);
}

off_t SkPDFPage::getPageSize(SkPDFCatalog* catalog, off_t fileOffset) {
    SkASSERT(fContentStream.get() != NULL);
    catalog->setFileOffset(fContentStream.get(), fileOffset);
    return fContentStream->getOutputSize(catalog, true);
}

void SkPDFPage::emitPage(SkWStream* stream, SkPDFCatalog* catalog) {
    SkASSERT(fContentStream.get() != NULL);
    fContentStream->emitObject(stream, catalog, true);
}


void SkPDFPage::GeneratePageTree(const SkTDArray<SkPDFPage*>& pages,
                                 SkPDFCatalog* catalog,
                                 SkTDArray<SkPDFDict*>* pageTree,
                                 SkPDFDict** rootNode) {
    
    
    
    
    
    
    
    static const int kNodeSize = 8;

    SkRefPtr<SkPDFName> kidsName = new SkPDFName("Kids");
    kidsName->unref();  
    SkRefPtr<SkPDFName> countName = new SkPDFName("Count");
    countName->unref();  
    SkRefPtr<SkPDFName> parentName = new SkPDFName("Parent");
    parentName->unref();  

    
    SkTDArray<SkPDFDict*> curNodes;
    curNodes.setReserve(pages.count());
    for (int i = 0; i < pages.count(); i++) {
        SkSafeRef(pages[i]);
        curNodes.push(pages[i]);
    }

    
    SkTDArray<SkPDFDict*> nextRoundNodes;
    nextRoundNodes.setReserve((pages.count() + kNodeSize - 1)/kNodeSize);

    int treeCapacity = kNodeSize;
    do {
        for (int i = 0; i < curNodes.count(); ) {
            if (i > 0 && i + 1 == curNodes.count()) {
                nextRoundNodes.push(curNodes[i]);
                break;
            }

            SkPDFDict* newNode = new SkPDFDict("Pages");
            SkRefPtr<SkPDFObjRef> newNodeRef = new SkPDFObjRef(newNode);
            newNodeRef->unref();  

            SkRefPtr<SkPDFArray> kids = new SkPDFArray;
            kids->unref();  
            kids->reserve(kNodeSize);

            int count = 0;
            for (; i < curNodes.count() && count < kNodeSize; i++, count++) {
                curNodes[i]->insert(parentName.get(), newNodeRef.get());
                kids->append(new SkPDFObjRef(curNodes[i]))->unref();

                
                
                if (curNodes[i] != pages[0]) {
                    pageTree->push(curNodes[i]);  
                    catalog->addObject(curNodes[i], false);
                } else {
                    SkSafeUnref(curNodes[i]);
                    catalog->addObject(curNodes[i], true);
                }
            }

            newNode->insert(kidsName.get(), kids.get());
            int pageCount = treeCapacity;
            if (count < kNodeSize) {
                pageCount = pages.count() % treeCapacity;
            }
            newNode->insert(countName.get(), new SkPDFInt(pageCount))->unref();
            nextRoundNodes.push(newNode);  
        }

        curNodes = nextRoundNodes;
        nextRoundNodes.rewind();
        treeCapacity *= kNodeSize;
    } while (curNodes.count() > 1);

    pageTree->push(curNodes[0]);  
    catalog->addObject(curNodes[0], false);
    if (rootNode) {
        *rootNode = curNodes[0];
    }
}

const SkTDArray<SkPDFFont*>& SkPDFPage::getFontResources() const {
    return fDevice->getFontResources();
}

const SkPDFGlyphSetMap& SkPDFPage::getFontGlyphUsage() const {
    return fDevice->getFontGlyphUsage();
}
