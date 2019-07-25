








#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFDocument.h"
#include "SkPDFPage.h"
#include "SkPDFFont.h"
#include "SkStream.h"



void addResourcesToCatalog(int firstIndex, bool firstPage,
                          SkTDArray<SkPDFObject*>* resourceList,
                          SkPDFCatalog* catalog) {
    for (int i = firstIndex; i < resourceList->count(); i++) {
        int index = resourceList->find((*resourceList)[i]);
        if (index != i) {
            (*resourceList)[i]->unref();
            resourceList->removeShuffle(i);
            i--;
        } else {
            catalog->addObject((*resourceList)[i], firstPage);
        }
    }
}

static void perform_font_subsetting(SkPDFCatalog* catalog,
                                    const SkTDArray<SkPDFPage*>& pages,
                                    SkTDArray<SkPDFObject*>* substitutes) {
    SkASSERT(catalog);
    SkASSERT(substitutes);

    SkPDFGlyphSetMap usage;
    for (int i = 0; i < pages.count(); ++i) {
        usage.merge(pages[i]->getFontGlyphUsage());
    }
    SkPDFGlyphSetMap::F2BIter iterator(usage);
    SkPDFGlyphSetMap::FontGlyphSetPair* entry = iterator.next();
    while (entry) {
        SkPDFFont* subsetFont =
            entry->fFont->getFontSubset(entry->fGlyphSet);
        if (subsetFont) {
            catalog->setSubstitute(entry->fFont, subsetFont);
            substitutes->push(subsetFont);  
        }
        entry = iterator.next();
    }
}

SkPDFDocument::SkPDFDocument(Flags flags)
        : fXRefFileOffset(0),
          fSecondPageFirstResourceIndex(0) {
    fCatalog.reset(new SkPDFCatalog(flags));
    fDocCatalog = new SkPDFDict("Catalog");
    fDocCatalog->unref();  
    fCatalog->addObject(fDocCatalog.get(), true);
}

SkPDFDocument::~SkPDFDocument() {
    fPages.safeUnrefAll();

    
    
    for (int i = 0; i < fPageTree.count(); i++) {
        fPageTree[i]->clear();
    }
    fPageTree.safeUnrefAll();
    fPageResources.safeUnrefAll();
    fSubstitutes.safeUnrefAll();
}

bool SkPDFDocument::emitPDF(SkWStream* stream) {
    if (fPages.isEmpty()) {
        return false;
    }
    for (int i = 0; i < fPages.count(); i++) {
        if (fPages[i] == NULL) {
            return false;
        }
    }

    
    if (fPageTree.isEmpty()) {
        SkPDFDict* pageTreeRoot;
        SkPDFPage::GeneratePageTree(fPages, fCatalog.get(), &fPageTree,
                                    &pageTreeRoot);
        fDocCatalog->insert("Pages", new SkPDFObjRef(pageTreeRoot))->unref();

        











        bool firstPage = true;
        for (int i = 0; i < fPages.count(); i++) {
            int resourceCount = fPageResources.count();
            fPages[i]->finalizePage(fCatalog.get(), firstPage, &fPageResources);
            addResourcesToCatalog(resourceCount, firstPage, &fPageResources,
                                  fCatalog.get());
            if (i == 0) {
                firstPage = false;
                fSecondPageFirstResourceIndex = fPageResources.count();
            }
        }

        
        perform_font_subsetting(fCatalog.get(), fPages, &fSubstitutes);

        
        off_t fileOffset = headerSize();
        fileOffset += fCatalog->setFileOffset(fDocCatalog.get(), fileOffset);
        fileOffset += fCatalog->setFileOffset(fPages[0], fileOffset);
        fileOffset += fPages[0]->getPageSize(fCatalog.get(), fileOffset);
        for (int i = 0; i < fSecondPageFirstResourceIndex; i++) {
            fileOffset += fCatalog->setFileOffset(fPageResources[i],
                                                  fileOffset);
        }
        
        fileOffset += fCatalog->setSubstituteResourcesOffsets(fileOffset, true);
        if (fPages.count() > 1) {
            
            
        }

        for (int i = 0; i < fPageTree.count(); i++) {
            fileOffset += fCatalog->setFileOffset(fPageTree[i], fileOffset);
        }

        for (int i = 1; i < fPages.count(); i++) {
            fileOffset += fPages[i]->getPageSize(fCatalog.get(), fileOffset);
        }

        for (int i = fSecondPageFirstResourceIndex;
                 i < fPageResources.count();
                 i++) {
            fileOffset += fCatalog->setFileOffset(fPageResources[i],
                                                  fileOffset);
        }

        fileOffset += fCatalog->setSubstituteResourcesOffsets(fileOffset,
                                                              false);
        fXRefFileOffset = fileOffset;
    }

    emitHeader(stream);
    fDocCatalog->emitObject(stream, fCatalog.get(), true);
    fPages[0]->emitObject(stream, fCatalog.get(), true);
    fPages[0]->emitPage(stream, fCatalog.get());
    for (int i = 0; i < fSecondPageFirstResourceIndex; i++) {
        fPageResources[i]->emit(stream, fCatalog.get(), true);
    }
    fCatalog->emitSubstituteResources(stream, true);
    
    
    
    
    

    for (int i = 0; i < fPageTree.count(); i++) {
        fPageTree[i]->emitObject(stream, fCatalog.get(), true);
    }

    for (int i = 1; i < fPages.count(); i++) {
        fPages[i]->emitPage(stream, fCatalog.get());
    }

    for (int i = fSecondPageFirstResourceIndex;
            i < fPageResources.count();
            i++) {
        fPageResources[i]->emit(stream, fCatalog.get(), true);
    }

    fCatalog->emitSubstituteResources(stream, false);
    int64_t objCount = fCatalog->emitXrefTable(stream, fPages.count() > 1);
    emitFooter(stream, objCount);
    return true;
}

bool SkPDFDocument::setPage(int pageNumber, SkPDFDevice* pdfDevice) {
    if (!fPageTree.isEmpty()) {
        return false;
    }

    pageNumber--;
    SkASSERT(pageNumber >= 0);

    if (pageNumber >= fPages.count()) {
        int oldSize = fPages.count();
        fPages.setCount(pageNumber + 1);
        for (int i = oldSize; i <= pageNumber; i++) {
            fPages[i] = NULL;
        }
    }

    SkPDFPage* page = new SkPDFPage(pdfDevice);
    SkSafeUnref(fPages[pageNumber]);
    fPages[pageNumber] = page;  
    return true;
}

bool SkPDFDocument::appendPage(SkPDFDevice* pdfDevice) {
    if (!fPageTree.isEmpty()) {
        return false;
    }

    SkPDFPage* page = new SkPDFPage(pdfDevice);
    fPages.push(page);  
    return true;
}

const SkTDArray<SkPDFPage*>& SkPDFDocument::getPages() {
    return fPages;
}

void SkPDFDocument::emitHeader(SkWStream* stream) {
    stream->writeText("%PDF-1.4\n%");
    
    
    stream->write32(0xD3EBE9E1);
    stream->writeText("\n");
}

size_t SkPDFDocument::headerSize() {
    SkDynamicMemoryWStream buffer;
    emitHeader(&buffer);
    return buffer.getOffset();
}

void SkPDFDocument::emitFooter(SkWStream* stream, int64_t objCount) {
    if (fTrailerDict.get() == NULL) {
        fTrailerDict = new SkPDFDict();
        fTrailerDict->unref();  

        
        
        fTrailerDict->insertInt("Size", objCount);
        fTrailerDict->insert("Root",
                             new SkPDFObjRef(fDocCatalog.get()))->unref();
    }

    stream->writeText("trailer\n");
    fTrailerDict->emitObject(stream, fCatalog.get(), false);
    stream->writeText("\nstartxref\n");
    stream->writeBigDecAsText(fXRefFileOffset);
    stream->writeText("\n%%EOF");
}
