








#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFDocument.h"
#include "SkPDFFont.h"
#include "SkPDFPage.h"
#include "SkPDFTypes.h"
#include "SkStream.h"
#include "SkTSet.h"

static void addResourcesToCatalog(bool firstPage,
                                  SkTSet<SkPDFObject*>* resourceSet,
                                  SkPDFCatalog* catalog) {
    for (int i = 0; i < resourceSet->count(); i++) {
        catalog->addObject((*resourceSet)[i], firstPage);
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
    const SkPDFGlyphSetMap::FontGlyphSetPair* entry = iterator.next();
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
          fTrailerDict(NULL) {
    fCatalog.reset(new SkPDFCatalog(flags));
    fDocCatalog = SkNEW_ARGS(SkPDFDict, ("Catalog"));
    fCatalog->addObject(fDocCatalog, true);
    fFirstPageResources = NULL;
    fOtherPageResources = NULL;
}

SkPDFDocument::~SkPDFDocument() {
    fPages.safeUnrefAll();

    
    
    for (int i = 0; i < fPageTree.count(); i++) {
        fPageTree[i]->clear();
    }
    fPageTree.safeUnrefAll();

    if (fFirstPageResources) {
        fFirstPageResources->safeUnrefAll();
    }
    if (fOtherPageResources) {
        fOtherPageResources->safeUnrefAll();
    }

    fSubstitutes.safeUnrefAll();

    fDocCatalog->unref();
    SkSafeUnref(fTrailerDict);
    SkDELETE(fFirstPageResources);
    SkDELETE(fOtherPageResources);
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

    fFirstPageResources = SkNEW(SkTSet<SkPDFObject*>);
    fOtherPageResources = SkNEW(SkTSet<SkPDFObject*>);

    
    if (fPageTree.isEmpty()) {
        SkPDFDict* pageTreeRoot;
        SkPDFPage::GeneratePageTree(fPages, fCatalog.get(), &fPageTree,
                                    &pageTreeRoot);
        fDocCatalog->insert("Pages", new SkPDFObjRef(pageTreeRoot))->unref();

        









        SkAutoTUnref<SkPDFDict> dests(SkNEW(SkPDFDict));

        bool firstPage = true;
        




        SkTSet<SkPDFObject*> knownResources;

        
        
        SkDEBUGCODE(int duplicates =) knownResources.mergeInto(*fFirstPageResources);
        SkASSERT(duplicates == 0);

        for (int i = 0; i < fPages.count(); i++) {
            if (i == 1) {
                firstPage = false;
                SkDEBUGCODE(duplicates =) knownResources.mergeInto(*fOtherPageResources);
            }
            SkTSet<SkPDFObject*> newResources;
            fPages[i]->finalizePage(
                fCatalog.get(), firstPage, knownResources, &newResources);
            addResourcesToCatalog(firstPage, &newResources, fCatalog.get());
            if (firstPage) {
                SkDEBUGCODE(duplicates =) fFirstPageResources->mergeInto(newResources);
            } else {
                SkDEBUGCODE(duplicates =) fOtherPageResources->mergeInto(newResources);
            }
            SkASSERT(duplicates == 0);

            SkDEBUGCODE(duplicates =) knownResources.mergeInto(newResources);
            SkASSERT(duplicates == 0);

            fPages[i]->appendDestinations(dests);
        }

        if (dests->size() > 0) {
            SkPDFDict* raw_dests = dests.get();
            fFirstPageResources->add(dests.detach());  
            fCatalog->addObject(raw_dests, true );
            fDocCatalog->insert("Dests", SkNEW_ARGS(SkPDFObjRef, (raw_dests)))->unref();
        }

        
        perform_font_subsetting(fCatalog.get(), fPages, &fSubstitutes);

        
        off_t fileOffset = headerSize();
        fileOffset += fCatalog->setFileOffset(fDocCatalog, fileOffset);
        fileOffset += fCatalog->setFileOffset(fPages[0], fileOffset);
        fileOffset += fPages[0]->getPageSize(fCatalog.get(),
                (size_t) fileOffset);
        for (int i = 0; i < fFirstPageResources->count(); i++) {
            fileOffset += fCatalog->setFileOffset((*fFirstPageResources)[i],
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

        for (int i = 0; i < fOtherPageResources->count(); i++) {
            fileOffset += fCatalog->setFileOffset(
                (*fOtherPageResources)[i], fileOffset);
        }

        fileOffset += fCatalog->setSubstituteResourcesOffsets(fileOffset,
                                                              false);
        fXRefFileOffset = fileOffset;
    }

    emitHeader(stream);
    fDocCatalog->emitObject(stream, fCatalog.get(), true);
    fPages[0]->emitObject(stream, fCatalog.get(), true);
    fPages[0]->emitPage(stream, fCatalog.get());
    for (int i = 0; i < fFirstPageResources->count(); i++) {
        (*fFirstPageResources)[i]->emit(stream, fCatalog.get(), true);
    }
    fCatalog->emitSubstituteResources(stream, true);
    
    
    
    
    

    for (int i = 0; i < fPageTree.count(); i++) {
        fPageTree[i]->emitObject(stream, fCatalog.get(), true);
    }

    for (int i = 1; i < fPages.count(); i++) {
        fPages[i]->emitPage(stream, fCatalog.get());
    }

    for (int i = 0; i < fOtherPageResources->count(); i++) {
        (*fOtherPageResources)[i]->emit(stream, fCatalog.get(), true);
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


void SkPDFDocument::getCountOfFontTypes(
        int counts[SkAdvancedTypefaceMetrics::kOther_Font + 2]) const {
    sk_bzero(counts, sizeof(int) *
                     (SkAdvancedTypefaceMetrics::kOther_Font + 2));
    SkTDArray<SkFontID> seenFonts;
    int notEmbeddable = 0;

    for (int pageNumber = 0; pageNumber < fPages.count(); pageNumber++) {
        const SkTDArray<SkPDFFont*>& fontResources =
                fPages[pageNumber]->getFontResources();
        for (int font = 0; font < fontResources.count(); font++) {
            SkFontID fontID = fontResources[font]->typeface()->uniqueID();
            if (seenFonts.find(fontID) == -1) {
                counts[fontResources[font]->getType()]++;
                seenFonts.push(fontID);
                if (!fontResources[font]->canEmbed()) {
                    notEmbeddable++;
                }
            }
        }
    }
    counts[SkAdvancedTypefaceMetrics::kOther_Font + 1] = notEmbeddable;
}

void SkPDFDocument::getCountOfFontTypes(
        int counts[SkAdvancedTypefaceMetrics::kOther_Font + 1],
        int* notSubsettableCount,
        int* notEmbeddableCount) const {
    sk_bzero(counts, sizeof(int) *
                     (SkAdvancedTypefaceMetrics::kOther_Font + 1));
    SkTDArray<SkFontID> seenFonts;
    int notSubsettable = 0;
    int notEmbeddable = 0;

    for (int pageNumber = 0; pageNumber < fPages.count(); pageNumber++) {
        const SkTDArray<SkPDFFont*>& fontResources =
                fPages[pageNumber]->getFontResources();
        for (int font = 0; font < fontResources.count(); font++) {
            SkFontID fontID = fontResources[font]->typeface()->uniqueID();
            if (seenFonts.find(fontID) == -1) {
                counts[fontResources[font]->getType()]++;
                seenFonts.push(fontID);
                if (!fontResources[font]->canSubset()) {
                    notSubsettable++;
                }
                if (!fontResources[font]->canEmbed()) {
                    notEmbeddable++;
                }
            }
        }
    }
    if (notSubsettableCount) {
        *notSubsettableCount = notSubsettable;

    }
    if (notEmbeddableCount) {
        *notEmbeddableCount = notEmbeddable;
    }
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
    if (NULL == fTrailerDict) {
        fTrailerDict = SkNEW(SkPDFDict);

        
        
        fTrailerDict->insertInt("Size", int(objCount));
        fTrailerDict->insert("Root", new SkPDFObjRef(fDocCatalog))->unref();
    }

    stream->writeText("trailer\n");
    fTrailerDict->emitObject(stream, fCatalog.get(), false);
    stream->writeText("\nstartxref\n");
    stream->writeBigDecAsText(fXRefFileOffset);
    stream->writeText("\n%%EOF");
}
