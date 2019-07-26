







#include "GrTextureStripAtlas.h"
#include "SkPixelRef.h"
#include "SkTSearch.h"
#include "GrBinHashKey.h"
#include "GrTexture.h"

#ifdef SK_DEBUG
    #define VALIDATE this->validate()
#else
    #define VALIDATE
#endif

GR_DEFINE_RESOURCE_CACHE_DOMAIN(GrTextureStripAtlas, GetTextureStripAtlasDomain)

int32_t GrTextureStripAtlas::gCacheCount = 0;


class AtlasEntry;
typedef GrTBinHashKey<AtlasEntry, sizeof(GrTextureStripAtlas::Desc)> AtlasHashKey;
class AtlasEntry : public ::GrNoncopyable {
public:
    AtlasEntry() : fAtlas(NULL) {}
    ~AtlasEntry() { SkDELETE(fAtlas); }
    int compare(const AtlasHashKey& key) const { return fKey.compare(key); }
    AtlasHashKey fKey;
    GrTextureStripAtlas* fAtlas;
};

GrTextureStripAtlas* GrTextureStripAtlas::GetAtlas(const GrTextureStripAtlas::Desc& desc) {
    static SkTDArray<AtlasEntry> gAtlasEntries;
    static GrTHashTable<AtlasEntry, AtlasHashKey, 8> gAtlasCache;
    AtlasHashKey key;
    key.setKeyData(desc.asKey());
    AtlasEntry* entry = gAtlasCache.find(key);
    if (NULL != entry) {
        return entry->fAtlas;
    } else {
        entry = gAtlasEntries.push();
        entry->fAtlas = SkNEW_ARGS(GrTextureStripAtlas, (desc));
        entry->fKey = key;
        gAtlasCache.insert(key, entry);
        return entry->fAtlas;
    }
}

GrTextureStripAtlas::GrTextureStripAtlas(GrTextureStripAtlas::Desc desc)
    : fCacheID(sk_atomic_inc(&gCacheCount))
    , fLockedRows(0)
    , fDesc(desc)
    , fNumRows(desc.fHeight / desc.fRowHeight)
    , fTexture(NULL)
    , fRows(SkNEW_ARRAY(AtlasRow, fNumRows))
    , fLRUFront(NULL)
    , fLRUBack(NULL) {
    GrAssert(fNumRows * fDesc.fRowHeight == fDesc.fHeight);
    this->initLRU();
    VALIDATE;
}

GrTextureStripAtlas::~GrTextureStripAtlas() {
    SkDELETE_ARRAY(fRows);
}

int GrTextureStripAtlas::lockRow(const SkBitmap& data) {
    VALIDATE;
    if (0 == fLockedRows) {
        this->lockTexture();
    }

    int key = data.getGenerationID();
    int rowNumber = -1;
    int index = this->searchByKey(key);

    if (index >= 0) {
        
        AtlasRow* row = fKeyTable[index];
        if (0 == row->fLocks) {
            this->removeFromLRU(row);
        }
        ++row->fLocks;
        ++fLockedRows;

        
        
        rowNumber = static_cast<int>(row - fRows);
    } else {
        
        index = ~index;

        
        AtlasRow* row = this->getLRU();

        ++fLockedRows;

        if (NULL == row) {
            
            fDesc.fContext->flush();
            row = this->getLRU();
            if (NULL == row) {
                --fLockedRows;
                return -1;
            }
        }

        this->removeFromLRU(row);

        uint32_t oldKey = row->fKey;

        
        
        if (oldKey != kEmptyAtlasRowKey) {

            
            
            int oldIndex = this->searchByKey(oldKey);
            if (oldIndex < index) {
                --index;
            }

            fKeyTable.remove(oldIndex);
        }

        row->fKey = key;
        row->fLocks = 1;
        fKeyTable.insert(index, 1, &row);
        rowNumber = static_cast<int>(row - fRows);

        SkAutoLockPixels lock(data);

        
        
        fDesc.fContext->writeTexturePixels(fTexture,
                                           0,  rowNumber * fDesc.fRowHeight,
                                           fDesc.fWidth, fDesc.fRowHeight,
                                           SkBitmapConfig2GrPixelConfig(data.config()),
                                           data.getPixels(),
                                           data.rowBytes(),
                                           GrContext::kDontFlush_PixelOpsFlag);
    }

    GrAssert(rowNumber >= 0);
    VALIDATE;
    return rowNumber;
}

void GrTextureStripAtlas::unlockRow(int row) {
    VALIDATE;
    --fRows[row].fLocks;
    --fLockedRows;
    GrAssert(fRows[row].fLocks >= 0 && fLockedRows >= 0);
    if (0 == fRows[row].fLocks) {
        this->appendLRU(fRows + row);
    }
    if (0 == fLockedRows) {
        this->unlockTexture();
    }
    VALIDATE;
}

GrTextureStripAtlas::AtlasRow* GrTextureStripAtlas::getLRU() {
    
    AtlasRow* row = fLRUFront;
    return row;
}

void GrTextureStripAtlas::lockTexture() {
    GrTextureParams params;
    GrTextureDesc texDesc;
    texDesc.fWidth = fDesc.fWidth;
    texDesc.fHeight = fDesc.fHeight;
    texDesc.fConfig = fDesc.fConfig;
    GrCacheData cacheData(fCacheID);
    cacheData.fResourceDomain = GetTextureStripAtlasDomain();
    fTexture = fDesc.fContext->findTexture(texDesc, cacheData, &params);
    if (NULL == fTexture) {
        fTexture = fDesc.fContext->createTexture(&params, texDesc, cacheData, NULL, 0);
        
        this->initLRU();
        fKeyTable.rewind();
    }
    GrAssert(NULL != fTexture);
    fTexture->ref();
}

void GrTextureStripAtlas::unlockTexture() {
    GrAssert(NULL != fTexture && 0 == fLockedRows);
    fTexture->unref();
    fTexture = NULL;
    fDesc.fContext->purgeCache();
}

void GrTextureStripAtlas::initLRU() {
    fLRUFront = NULL;
    fLRUBack = NULL;
    
    for (int i = 0; i < fNumRows; ++i) {
        fRows[i].fKey = kEmptyAtlasRowKey;
        fRows[i].fNext = NULL;
        fRows[i].fPrev = NULL;
        this->appendLRU(fRows + i);
    }
    GrAssert(NULL == fLRUFront->fPrev && NULL == fLRUBack->fNext);
}

void GrTextureStripAtlas::appendLRU(AtlasRow* row) {
    GrAssert(NULL == row->fPrev && NULL == row->fNext);
    if (NULL == fLRUFront && NULL == fLRUBack) {
        fLRUFront = row;
        fLRUBack = row;
    } else {
        row->fPrev = fLRUBack;
        fLRUBack->fNext = row;
        fLRUBack = row;
    }
}

void GrTextureStripAtlas::removeFromLRU(AtlasRow* row) {
    GrAssert(NULL != row);
    if (NULL != row->fNext && NULL != row->fPrev) {
        row->fPrev->fNext = row->fNext;
        row->fNext->fPrev = row->fPrev;
    } else {
        if (NULL == row->fNext) {
            GrAssert(row == fLRUBack);
            fLRUBack = row->fPrev;
            if (fLRUBack) {
                fLRUBack->fNext = NULL;
            }
        }
        if (NULL == row->fPrev) {
            GrAssert(row == fLRUFront);
            fLRUFront = row->fNext;
            if (fLRUFront) {
                fLRUFront->fPrev = NULL;
            }
        }
    }
    row->fNext = NULL;
    row->fPrev = NULL;
}

int GrTextureStripAtlas::searchByKey(uint32_t key) {
    AtlasRow target;
    target.fKey = key;
    return SkTSearch<AtlasRow, GrTextureStripAtlas::compareKeys>((const AtlasRow**)fKeyTable.begin(),
                                                                 fKeyTable.count(),
                                                                 &target,
                                                                 sizeof(AtlasRow*));
}

#ifdef SK_DEBUG
void GrTextureStripAtlas::validate() {

    
    uint32_t prev = 1 > fKeyTable.count() ? 0 : fKeyTable[0]->fKey;
    for (int i = 1; i < fKeyTable.count(); ++i) {
        GrAssert(prev < fKeyTable[i]->fKey);
        GrAssert(fKeyTable[i]->fKey != kEmptyAtlasRowKey);
        prev = fKeyTable[i]->fKey;
    }

    int lruCount = 0;
    
    GrAssert(NULL == fLRUFront || NULL == fLRUFront->fPrev);
    GrAssert(NULL == fLRUBack  || NULL == fLRUBack->fNext);
    for (AtlasRow* r = fLRUFront; r != NULL; r = r->fNext) {
        if (NULL == r->fNext) {
            GrAssert(r == fLRUBack);
        } else {
            GrAssert(r->fNext->fPrev == r);
        }
        ++lruCount;
    }

    int rowLocks = 0;
    int freeRows = 0;

    for (int i = 0; i < fNumRows; ++i) {
        rowLocks += fRows[i].fLocks;
        if (0 == fRows[i].fLocks) {
            ++freeRows;
            bool inLRU = false;
            
            for (AtlasRow* r = fLRUFront; r != NULL; r = r->fNext) {
                if (r == &fRows[i]) {
                    inLRU = true;
                    break;
                }
            }
            GrAssert(inLRU);
        } else {
            
            GrAssert(kEmptyAtlasRowKey != fRows[i].fKey);
        }

        
        GrAssert(fRows[i].fKey == kEmptyAtlasRowKey || this->searchByKey(fRows[i].fKey) >= 0);
    }

    
    
    
    GrAssert(rowLocks == fLockedRows || rowLocks + 1 == fLockedRows);

    
    GrAssert(freeRows == lruCount);

    
    
    if (fLockedRows == 0) {
        GrAssert(NULL == fTexture);
    } else {
        GrAssert(NULL != fTexture);
    }
}
#endif

