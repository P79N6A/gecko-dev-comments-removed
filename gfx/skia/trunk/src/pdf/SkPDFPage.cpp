








#include "SkData.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFPage.h"
#include "SkPDFResourceDict.h"

SkPDFPage::SkPDFPage(SkPDFDevice* content)
    : SkPDFDict("Page"),
      fDevice(content) {
  SkSafeRef(content);
}

SkPDFPage::~SkPDFPage() {}

void SkPDFPage::finalizePage(SkPDFCatalog* catalog, bool firstPage,
                             const SkTSet<SkPDFObject*>& knownResourceObjects,
                             SkTSet<SkPDFObject*>* newResourceObjects) {
    SkPDFResourceDict* resourceDict = fDevice->getResourceDict();
    if (fContentStream.get() == NULL) {
        insert("Resources", resourceDict);
        SkSafeUnref(this->insert("MediaBox", fDevice->copyMediaBox()));
        if (!SkToBool(catalog->getDocumentFlags() &
                      SkPDFDocument::kNoLinks_Flags)) {
            SkPDFArray* annots = fDevice->getAnnotations();
            if (annots && annots->size() > 0) {
                insert("Annots", annots);
            }
        }

        SkAutoTUnref<SkData> content(fDevice->copyContentToData());
        fContentStream.reset(new SkPDFStream(content.get()));
        insert("Contents", new SkPDFObjRef(fContentStream.get()))->unref();
    }
    catalog->addObject(fContentStream.get(), firstPage);
    resourceDict->getReferencedResources(knownResourceObjects,
                                         newResourceObjects,
                                         true);
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

    SkAutoTUnref<SkPDFName> kidsName(new SkPDFName("Kids"));
    SkAutoTUnref<SkPDFName> countName(new SkPDFName("Count"));
    SkAutoTUnref<SkPDFName> parentName(new SkPDFName("Parent"));

    
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
            SkAutoTUnref<SkPDFObjRef> newNodeRef(new SkPDFObjRef(newNode));

            SkAutoTUnref<SkPDFArray> kids(new SkPDFArray);
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

            
            
            
            
            
            
            
            int pageCount = treeCapacity;
            if (i == curNodes.count()) {
                pageCount = ((pages.count() - 1) % treeCapacity) + 1;
            }
            newNode->insert(countName.get(), new SkPDFInt(pageCount))->unref();
            newNode->insert(kidsName.get(), kids.get());
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

void SkPDFPage::appendDestinations(SkPDFDict* dict) {
    fDevice->appendDestinations(dict, this);
}
