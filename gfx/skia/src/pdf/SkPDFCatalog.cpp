








#include "SkPDFCatalog.h"
#include "SkPDFTypes.h"
#include "SkStream.h"
#include "SkTypes.h"

SkPDFCatalog::SkPDFCatalog(SkPDFDocument::Flags flags)
    : fFirstPageCount(0),
      fNextObjNum(1),
      fNextFirstPageObjNum(0),
      fDocumentFlags(flags) {
}

SkPDFCatalog::~SkPDFCatalog() {
    fSubstituteResourcesRemaining.safeUnrefAll();
    fSubstituteResourcesFirstPage.safeUnrefAll();
}

SkPDFObject* SkPDFCatalog::addObject(SkPDFObject* obj, bool onFirstPage) {
    if (findObjectIndex(obj) != -1) {  
        return obj;
    }
    SkASSERT(fNextFirstPageObjNum == 0);
    if (onFirstPage) {
        fFirstPageCount++;
    }

    struct Rec newEntry(obj, onFirstPage);
    fCatalog.append(1, &newEntry);
    return obj;
}

size_t SkPDFCatalog::setFileOffset(SkPDFObject* obj, size_t offset) {
    int objIndex = assignObjNum(obj) - 1;
    SkASSERT(fCatalog[objIndex].fObjNumAssigned);
    SkASSERT(fCatalog[objIndex].fFileOffset == 0);
    fCatalog[objIndex].fFileOffset = offset;

    return getSubstituteObject(obj)->getOutputSize(this, true);
}

void SkPDFCatalog::emitObjectNumber(SkWStream* stream, SkPDFObject* obj) {
    stream->writeDecAsText(assignObjNum(obj));
    stream->writeText(" 0");  
}

size_t SkPDFCatalog::getObjectNumberSize(SkPDFObject* obj) {
    SkDynamicMemoryWStream buffer;
    emitObjectNumber(&buffer, obj);
    return buffer.getOffset();
}

int SkPDFCatalog::findObjectIndex(SkPDFObject* obj) const {
    for (int i = 0; i < fCatalog.count(); i++) {
        if (fCatalog[i].fObject == obj) {
            return i;
        }
    }
    
    for (int i = 0; i < fSubstituteMap.count(); ++i) {
        if (fSubstituteMap[i].fSubstitute == obj) {
            return findObjectIndex(fSubstituteMap[i].fOriginal);
        }
    }
    return -1;
}

int SkPDFCatalog::assignObjNum(SkPDFObject* obj) {
    int pos = findObjectIndex(obj);
    
    
    SkASSERT(pos >= 0);
    uint32_t currentIndex = pos;
    if (fCatalog[currentIndex].fObjNumAssigned) {
        return currentIndex + 1;
    }

    
    if (fNextFirstPageObjNum == 0) {
        fNextFirstPageObjNum = fCatalog.count() - fFirstPageCount + 1;
    }

    uint32_t objNum;
    if (fCatalog[currentIndex].fOnFirstPage) {
        objNum = fNextFirstPageObjNum;
        fNextFirstPageObjNum++;
    } else {
        objNum = fNextObjNum;
        fNextObjNum++;
    }

    
    
    SkASSERT(!fCatalog[objNum - 1].fObjNumAssigned);
    if (objNum - 1 != currentIndex) {
        SkTSwap(fCatalog[objNum - 1], fCatalog[currentIndex]);
    }
    fCatalog[objNum - 1].fObjNumAssigned = true;
    return objNum;
}

int32_t SkPDFCatalog::emitXrefTable(SkWStream* stream, bool firstPage) {
    int first = -1;
    int last = fCatalog.count() - 1;
    
    
    
    
    
    

    stream->writeText("xref\n");
    stream->writeDecAsText(first + 1);
    stream->writeText(" ");
    stream->writeDecAsText(last - first + 1);
    stream->writeText("\n");

    if (first == -1) {
        stream->writeText("0000000000 65535 f \n");
        first++;
    }
    for (int i = first; i <= last; i++) {
        SkASSERT(fCatalog[i].fFileOffset > 0);
        SkASSERT(fCatalog[i].fFileOffset <= 9999999999LL);
        stream->writeBigDecAsText(fCatalog[i].fFileOffset, 10);
        stream->writeText(" 00000 n \n");
    }

    return fCatalog.count() + 1;
}

void SkPDFCatalog::setSubstitute(SkPDFObject* original,
                                 SkPDFObject* substitute) {
#if defined(SK_DEBUG)
    
    for (int i = 0; i < fSubstituteMap.count(); ++i) {
        if (original == fSubstituteMap[i].fSubstitute ||
            original == fSubstituteMap[i].fOriginal) {
            SkASSERT(false);
            return;
        }
    }
#endif
    
    bool onFirstPage = false;
    for (int i = 0; i < fCatalog.count(); ++i) {
        if (fCatalog[i].fObject == original) {
            onFirstPage = fCatalog[i].fOnFirstPage;
            break;
        }
#if defined(SK_DEBUG)
        if (i == fCatalog.count() - 1) {
            SkASSERT(false);  
            return;
        }
#endif
    }

    SubstituteMapping newMapping(original, substitute);
    fSubstituteMap.append(1, &newMapping);

    
    SkTDArray<SkPDFObject*>* targetList = getSubstituteList(onFirstPage);
    int existingSize = targetList->count();
    newMapping.fSubstitute->getResources(targetList);
    for (int i = existingSize; i < targetList->count(); ++i) {
        addObject((*targetList)[i], onFirstPage);
    }
}

SkPDFObject* SkPDFCatalog::getSubstituteObject(SkPDFObject* object) {
    for (int i = 0; i < fSubstituteMap.count(); ++i) {
        if (object == fSubstituteMap[i].fOriginal) {
            return fSubstituteMap[i].fSubstitute;
        }
    }
    return object;
}

off_t SkPDFCatalog::setSubstituteResourcesOffsets(off_t fileOffset,
                                                  bool firstPage) {
    SkTDArray<SkPDFObject*>* targetList = getSubstituteList(firstPage);
    off_t offsetSum = fileOffset;
    for (int i = 0; i < targetList->count(); ++i) {
        offsetSum += setFileOffset((*targetList)[i], offsetSum);
    }
    return offsetSum - fileOffset;
}

void SkPDFCatalog::emitSubstituteResources(SkWStream *stream, bool firstPage) {
    SkTDArray<SkPDFObject*>* targetList = getSubstituteList(firstPage);
    for (int i = 0; i < targetList->count(); ++i) {
        (*targetList)[i]->emit(stream, this, true);
    }
}

SkTDArray<SkPDFObject*>* SkPDFCatalog::getSubstituteList(bool firstPage) {
    return firstPage ? &fSubstituteResourcesFirstPage :
                       &fSubstituteResourcesRemaining;
}
