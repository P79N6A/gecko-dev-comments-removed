







#include "SkBitmapHeap.h"

#include "SkBitmap.h"
#include "SkFlattenableBuffers.h"
#include "SkTSearch.h"

SK_DEFINE_INST_COUNT(SkBitmapHeapReader)
SK_DEFINE_INST_COUNT(SkBitmapHeap::ExternalStorage)

SkBitmapHeapEntry::SkBitmapHeapEntry()
    : fSlot(-1)
    , fRefCount(0)
    , fBytesAllocated(0) {
}

SkBitmapHeapEntry::~SkBitmapHeapEntry() {
    SkASSERT(0 == fRefCount);
}

void SkBitmapHeapEntry::addReferences(int count) {
    if (0 == fRefCount) {
        
        
        
        fRefCount = count;
    } else {
        sk_atomic_add(&fRefCount, count);
    }
}



int SkBitmapHeap::LookupEntry::Compare(const SkBitmapHeap::LookupEntry *a,
                                       const SkBitmapHeap::LookupEntry *b) {
    if (a->fGenerationId < b->fGenerationId) {
        return -1;
    } else if (a->fGenerationId > b->fGenerationId) {
        return 1;
    } else if (a->fPixelOffset < b->fPixelOffset) {
        return -1;
    } else if (a->fPixelOffset > b->fPixelOffset) {
        return 1;
    } else if (a->fWidth < b->fWidth) {
        return -1;
    } else if (a->fWidth > b->fWidth) {
        return 1;
    } else if (a->fHeight < b->fHeight) {
        return -1;
    } else if (a->fHeight > b->fHeight) {
        return 1;
    }
    return 0;
}



SkBitmapHeap::SkBitmapHeap(int32_t preferredSize, int32_t ownerCount)
    : INHERITED()
    , fExternalStorage(NULL)
    , fMostRecentlyUsed(NULL)
    , fLeastRecentlyUsed(NULL)
    , fPreferredCount(preferredSize)
    , fOwnerCount(ownerCount)
    , fBytesAllocated(0)
    , fDeferAddingOwners(false) {
}

SkBitmapHeap::SkBitmapHeap(ExternalStorage* storage, int32_t preferredSize)
    : INHERITED()
    , fExternalStorage(storage)
    , fMostRecentlyUsed(NULL)
    , fLeastRecentlyUsed(NULL)
    , fPreferredCount(preferredSize)
    , fOwnerCount(IGNORE_OWNERS)
    , fBytesAllocated(0)
    , fDeferAddingOwners(false) {
    SkSafeRef(storage);
}

SkBitmapHeap::~SkBitmapHeap() {
    SkDEBUGCODE(
    for (int i = 0; i < fStorage.count(); i++) {
        bool unused = false;
        for (int j = 0; j < fUnusedSlots.count(); j++) {
            if (fUnusedSlots[j] == fStorage[i]->fSlot) {
                unused = true;
                break;
            }
        }
        if (!unused) {
            fBytesAllocated -= fStorage[i]->fBytesAllocated;
        }
    }
    fBytesAllocated -= (fStorage.count() * sizeof(SkBitmapHeapEntry));
    )
    SkASSERT(0 == fBytesAllocated);
    fStorage.deleteAll();
    SkSafeUnref(fExternalStorage);
    fLookupTable.deleteAll();
}

SkTRefArray<SkBitmap>* SkBitmapHeap::extractBitmaps() const {
    const int size = fStorage.count();
    SkTRefArray<SkBitmap>* array = NULL;
    if (size > 0) {
        array = SkTRefArray<SkBitmap>::Create(size);
        for (int i = 0; i < size; i++) {
            
            array->writableAt(i) = fStorage[i]->fBitmap;
        }
    }
    return array;
}

void SkBitmapHeap::removeFromLRU(SkBitmapHeap::LookupEntry* entry) {
    if (fMostRecentlyUsed == entry) {
        fMostRecentlyUsed = entry->fLessRecentlyUsed;
        if (NULL == fMostRecentlyUsed) {
            SkASSERT(fLeastRecentlyUsed == entry);
            fLeastRecentlyUsed = NULL;
        } else {
            fMostRecentlyUsed->fMoreRecentlyUsed = NULL;
        }
    } else {
        
        if (fLeastRecentlyUsed == entry) {
            SkASSERT(entry->fMoreRecentlyUsed != NULL);
            fLeastRecentlyUsed = entry->fMoreRecentlyUsed;
        }
        
        
        SkASSERT(entry->fMoreRecentlyUsed != NULL);
        entry->fMoreRecentlyUsed->fLessRecentlyUsed = entry->fLessRecentlyUsed;

        if (entry->fLessRecentlyUsed != NULL) {
            SkASSERT(fLeastRecentlyUsed != entry);
            entry->fLessRecentlyUsed->fMoreRecentlyUsed = entry->fMoreRecentlyUsed;
        }
    }
    entry->fMoreRecentlyUsed = NULL;
}

void SkBitmapHeap::appendToLRU(SkBitmapHeap::LookupEntry* entry) {
    if (fMostRecentlyUsed != NULL) {
        SkASSERT(NULL == fMostRecentlyUsed->fMoreRecentlyUsed);
        fMostRecentlyUsed->fMoreRecentlyUsed = entry;
        entry->fLessRecentlyUsed = fMostRecentlyUsed;
    }
    fMostRecentlyUsed = entry;
    if (NULL == fLeastRecentlyUsed) {
        fLeastRecentlyUsed = entry;
    }
}


SkBitmapHeap::LookupEntry* SkBitmapHeap::findEntryToReplace(const SkBitmap& replacement) {
    SkASSERT(fPreferredCount != UNLIMITED_SIZE);
    SkASSERT(fStorage.count() >= fPreferredCount);

    SkBitmapHeap::LookupEntry* iter = fLeastRecentlyUsed;
    while (iter != NULL) {
        SkBitmapHeapEntry* heapEntry = fStorage[iter->fStorageSlot];
        if (heapEntry->fRefCount > 0) {
            
            
            
            return NULL;
        }
        if (replacement.getGenerationID() == iter->fGenerationId) {
            
            
            
            iter = iter->fMoreRecentlyUsed;
        } else {
            return iter;
        }
    }
    return NULL;
}

size_t SkBitmapHeap::freeMemoryIfPossible(size_t bytesToFree) {
    if (UNLIMITED_SIZE == fPreferredCount) {
        return 0;
    }
    LookupEntry* iter = fLeastRecentlyUsed;
    size_t origBytesAllocated = fBytesAllocated;
    
    
    while (iter != NULL) {
        SkBitmapHeapEntry* heapEntry = fStorage[iter->fStorageSlot];
        if (heapEntry->fRefCount > 0) {
            break;
        }
        LookupEntry* next = iter->fMoreRecentlyUsed;
        this->removeEntryFromLookupTable(iter);
        
        
        heapEntry->fBitmap.reset();
        
        fUnusedSlots.push(heapEntry->fSlot);
        iter = next;
        if (origBytesAllocated - fBytesAllocated >= bytesToFree) {
            break;
        }
    }

    if (fLeastRecentlyUsed != iter) {
        
        fLeastRecentlyUsed = iter;
        if (NULL == fLeastRecentlyUsed) {
            
            fMostRecentlyUsed = NULL;
            fBytesAllocated -= (fStorage.count() * sizeof(SkBitmapHeapEntry));
            fStorage.deleteAll();
            fUnusedSlots.reset();
            SkASSERT(0 == fBytesAllocated);
        } else {
            fLeastRecentlyUsed->fLessRecentlyUsed = NULL;
        }
    }

    return origBytesAllocated - fBytesAllocated;
}

int SkBitmapHeap::findInLookupTable(const LookupEntry& indexEntry, SkBitmapHeapEntry** entry) {
    int index = SkTSearch<const LookupEntry>((const LookupEntry**)fLookupTable.begin(),
                                             fLookupTable.count(),
                                             &indexEntry, sizeof(void*), LookupEntry::Compare);

    if (index < 0) {
        
        index = ~index;
        *fLookupTable.insert(index) = SkNEW_ARGS(LookupEntry, (indexEntry));
    } else if (entry != NULL) {
        
        *entry = fStorage[fLookupTable[index]->fStorageSlot];
    }

    return index;
}

bool SkBitmapHeap::copyBitmap(const SkBitmap& originalBitmap, SkBitmap& copiedBitmap) {
    SkASSERT(!fExternalStorage);

    
    
    if (originalBitmap.isImmutable()) {
        copiedBitmap = originalBitmap;




    } else if (originalBitmap.empty()) {
        copiedBitmap.reset();
    } else if (!originalBitmap.deepCopyTo(&copiedBitmap, originalBitmap.getConfig())) {
        return false;
    }
    copiedBitmap.setImmutable();
    return true;
}

int SkBitmapHeap::removeEntryFromLookupTable(LookupEntry* entry) {
    
    SkDEBUGCODE(int count = fLookupTable.count();)
    int index = this->findInLookupTable(*entry, NULL);
    
    
    SkASSERT(count == fLookupTable.count());
    fBytesAllocated -= fStorage[entry->fStorageSlot]->fBytesAllocated;
    SkDELETE(fLookupTable[index]);
    fLookupTable.remove(index);
    return index;
}

int32_t SkBitmapHeap::insert(const SkBitmap& originalBitmap) {
    SkBitmapHeapEntry* entry = NULL;
    int searchIndex = this->findInLookupTable(LookupEntry(originalBitmap), &entry);

    if (entry) {
        
        if (fOwnerCount != IGNORE_OWNERS) {
            if (fDeferAddingOwners) {
                *fDeferredEntries.append() = entry->fSlot;
            } else {
                entry->addReferences(fOwnerCount);
            }
        }
        if (fPreferredCount != UNLIMITED_SIZE) {
            LookupEntry* lookupEntry = fLookupTable[searchIndex];
            if (lookupEntry != fMostRecentlyUsed) {
                this->removeFromLRU(lookupEntry);
                this->appendToLRU(lookupEntry);
            }
        }
        return entry->fSlot;
    }

    
    if (fPreferredCount != UNLIMITED_SIZE && fStorage.count() >= fPreferredCount) {
        
        LookupEntry* lookupEntry = this->findEntryToReplace(originalBitmap);
        if (lookupEntry != NULL) {
            
            entry = fStorage[lookupEntry->fStorageSlot];
            
            this->removeFromLRU(lookupEntry);
            int index = this->removeEntryFromLookupTable(lookupEntry);

            
            if (index < searchIndex) {
                searchIndex--;
            }
        }
    }

    
    if (!entry) {
        if (fPreferredCount != UNLIMITED_SIZE && fUnusedSlots.count() > 0) {
            int slot;
            fUnusedSlots.pop(&slot);
            entry = fStorage[slot];
        } else {
            entry = SkNEW(SkBitmapHeapEntry);
            fStorage.append(1, &entry);
            entry->fSlot = fStorage.count() - 1;
            fBytesAllocated += sizeof(SkBitmapHeapEntry);
        }
    }

    
    bool copySucceeded;
    if (fExternalStorage) {
        copySucceeded = fExternalStorage->insert(originalBitmap, entry->fSlot);
    } else {
        copySucceeded = copyBitmap(originalBitmap, entry->fBitmap);
    }

    
    if (!copySucceeded) {
        
        SkDELETE(fLookupTable[searchIndex]);
        fLookupTable.remove(searchIndex);
        
        if (fStorage.count() - 1 == entry->fSlot) {
            
            fStorage.remove(entry->fSlot);
            fBytesAllocated -= sizeof(SkBitmapHeapEntry);
            SkDELETE(entry);
        } else {
            fUnusedSlots.push(entry->fSlot);
        }
        return INVALID_SLOT;
    }

    
    fLookupTable[searchIndex]->fStorageSlot = entry->fSlot;

    
    
    
    
    entry->fBytesAllocated += originalBitmap.getSize();

    
    fBytesAllocated += entry->fBytesAllocated;

    if (fOwnerCount != IGNORE_OWNERS) {
        entry->addReferences(fOwnerCount);
    }
    if (fPreferredCount != UNLIMITED_SIZE) {
        this->appendToLRU(fLookupTable[searchIndex]);
    }
    return entry->fSlot;
}

void SkBitmapHeap::deferAddingOwners() {
    fDeferAddingOwners = true;
}

void SkBitmapHeap::endAddingOwnersDeferral(bool add) {
    if (add) {
        for (int i = 0; i < fDeferredEntries.count(); i++) {
            SkASSERT(fOwnerCount != IGNORE_OWNERS);
            SkBitmapHeapEntry* heapEntry = this->getEntry(fDeferredEntries[i]);
            SkASSERT(heapEntry != NULL);
            heapEntry->addReferences(fOwnerCount);
        }
    }
    fDeferAddingOwners = false;
    fDeferredEntries.reset();
}
