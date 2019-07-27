






#include "SkFontMgr_indirect.h"

#include "SkDataTable.h"
#include "SkFontStyle.h"
#include "SkOnce.h"
#include "SkStream.h"
#include "SkTSearch.h"
#include "SkTypeface.h"

class SkData;
class SkString;

class SkStyleSet_Indirect : public SkFontStyleSet {
public:
    
    SkStyleSet_Indirect(const SkFontMgr_Indirect* owner, int familyIndex,
                        SkRemotableFontIdentitySet* data)
        : fOwner(SkRef(owner)), fFamilyIndex(familyIndex), fData(data)
    { }

    virtual int count() SK_OVERRIDE { return fData->count(); }

    virtual void getStyle(int index, SkFontStyle* fs, SkString* style) SK_OVERRIDE {
        if (fs) {
            *fs = fData->at(index).fFontStyle;
        }
        if (style) {
            
            style->reset();
        }
    }

    virtual SkTypeface* createTypeface(int index) SK_OVERRIDE {
        return fOwner->createTypefaceFromFontId(fData->at(index));
    }

    virtual SkTypeface* matchStyle(const SkFontStyle& pattern) SK_OVERRIDE {
        if (fFamilyIndex >= 0) {
            SkFontIdentity id = fOwner->fProxy->matchIndexStyle(fFamilyIndex, pattern);
            return fOwner->createTypefaceFromFontId(id);
        }

        
        
        

        
        struct Score {
            int score;
            int index;
        };

        
        
        
        
        

        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        

        Score maxScore = { 0, 0 };
        for (int i = 0; i < fData->count(); ++i) {
            const SkFontStyle& current = fData->at(i).fFontStyle;
            Score currentScore = { 0, i };

            
            
            if (pattern.width() <= SkFontStyle::kNormal_Width) {
                if (current.width() <= pattern.width()) {
                    currentScore.score += 10 - pattern.width() + current.width();
                } else {
                    currentScore.score += 10 - current.width();
                }
            } else {
                if (current.width() > pattern.width()) {
                    currentScore.score += 10 + pattern.width() - current.width();
                } else {
                    currentScore.score += current.width();
                }
            }
            currentScore.score *= 1002;

            
            
            
            if (pattern.isItalic() && current.isItalic()) {
                currentScore.score += 1001;
            }

            

            
            
            if (pattern.weight() == current.weight()) {
                currentScore.score += 1000;
            } else if (pattern.weight() <= 500) {
                if (pattern.weight() >= 400 && pattern.weight() < 450) {
                    if (current.weight() >= 450 && current.weight() <= 500) {
                        
                        
                        currentScore.score += 500;
                    }
                }
                if (current.weight() <= pattern.weight()) {
                    currentScore.score += 1000 - pattern.weight() + current.weight();
                } else {
                    currentScore.score += 1000 - current.weight();
                }
            } else if (pattern.weight() > 500) {
                if (current.weight() > pattern.weight()) {
                    currentScore.score += 1000 + pattern.weight() - current.weight();
                } else {
                    currentScore.score += current.weight();
                }
            }

            if (currentScore.score > maxScore.score) {
                maxScore = currentScore;
            }
        }

        return this->createTypeface(maxScore.index);
    }
private:
    SkAutoTUnref<const SkFontMgr_Indirect> fOwner;
    int fFamilyIndex;
    SkAutoTUnref<SkRemotableFontIdentitySet> fData;
};

void SkFontMgr_Indirect::set_up_family_names(const SkFontMgr_Indirect* self) {
    self->fFamilyNames.reset(self->fProxy->getFamilyNames());
}

int SkFontMgr_Indirect::onCountFamilies() const {
    SkOnce(&fFamilyNamesInited, &fFamilyNamesMutex, SkFontMgr_Indirect::set_up_family_names, this);
    return fFamilyNames->count();
}

void SkFontMgr_Indirect::onGetFamilyName(int index, SkString* familyName) const {
    SkOnce(&fFamilyNamesInited, &fFamilyNamesMutex, SkFontMgr_Indirect::set_up_family_names, this);
    if (index >= fFamilyNames->count()) {
        familyName->reset();
        return;
    }
    familyName->set(fFamilyNames->atStr(index));
}

SkFontStyleSet* SkFontMgr_Indirect::onCreateStyleSet(int index) const {
    SkRemotableFontIdentitySet* set = fProxy->getIndex(index);
    if (NULL == set) {
        return NULL;
    }
    return SkNEW_ARGS(SkStyleSet_Indirect, (this, index, set));
}

SkFontStyleSet* SkFontMgr_Indirect::onMatchFamily(const char familyName[]) const {
    return SkNEW_ARGS(SkStyleSet_Indirect, (this, -1, fProxy->matchName(familyName)));
}

SkTypeface* SkFontMgr_Indirect::createTypefaceFromFontId(const SkFontIdentity& id) const {
    if (id.fDataId == SkFontIdentity::kInvalidDataId) {
        return NULL;
    }

    SkAutoMutexAcquire ama(fDataCacheMutex);

    SkAutoTUnref<SkTypeface> dataTypeface;
    int dataTypefaceIndex = 0;
    for (int i = 0; i < fDataCache.count(); ++i) {
        const DataEntry& entry = fDataCache[i];
        if (entry.fDataId == id.fDataId) {
            if (entry.fTtcIndex == id.fTtcIndex &&
                !entry.fTypeface->weak_expired() && entry.fTypeface->try_ref())
            {
                return entry.fTypeface;
            }
            if (dataTypeface.get() == NULL &&
                !entry.fTypeface->weak_expired() && entry.fTypeface->try_ref())
            {
                dataTypeface.reset(entry.fTypeface);
                dataTypefaceIndex = entry.fTtcIndex;
            }
        }

        if (entry.fTypeface->weak_expired()) {
            fDataCache.removeShuffle(i);
            --i;
        }
    }

    
    if (dataTypeface.get() != NULL) {
        SkAutoTUnref<SkStream> stream(dataTypeface->openStream(NULL));
        if (stream.get() != NULL) {
            return fImpl->createFromStream(stream.get(), dataTypefaceIndex);
        }
    }

    
    SkAutoTUnref<SkStreamAsset> stream(fProxy->getData(id.fDataId));
    if (stream.get() == NULL) {
        return NULL;
    }

    SkAutoTUnref<SkTypeface> typeface(fImpl->createFromStream(stream, id.fTtcIndex));
    if (typeface.get() == NULL) {
        return NULL;
    }

    DataEntry& newEntry = fDataCache.push_back();
    typeface->weak_ref();
    newEntry.fDataId = id.fDataId;
    newEntry.fTtcIndex = id.fTtcIndex;
    newEntry.fTypeface = typeface.get();  

    return typeface.detach();
}

SkTypeface* SkFontMgr_Indirect::onMatchFamilyStyle(const char familyName[],
                                                   const SkFontStyle& fontStyle) const {
    SkFontIdentity id = fProxy->matchNameStyle(familyName, fontStyle);
    return this->createTypefaceFromFontId(id);
}

SkTypeface* SkFontMgr_Indirect::onMatchFamilyStyleCharacter(const char familyName[],
                                                            const SkFontStyle& style,
                                                            const char bpc47[],
                                                            uint32_t character) const {
    SkFontIdentity id = fProxy->matchNameStyleCharacter(familyName, style, bpc47, character);
    return this->createTypefaceFromFontId(id);
}

SkTypeface* SkFontMgr_Indirect::onMatchFaceStyle(const SkTypeface* familyMember,
                                                 const SkFontStyle& fontStyle) const {
    SkString familyName;
    familyMember->getFamilyName(&familyName);
    return this->matchFamilyStyle(familyName.c_str(), fontStyle);
}

SkTypeface* SkFontMgr_Indirect::onCreateFromStream(SkStream* stream, int ttcIndex) const {
    return fImpl->createFromStream(stream, ttcIndex);
}

SkTypeface* SkFontMgr_Indirect::onCreateFromFile(const char path[], int ttcIndex) const {
    return fImpl->createFromFile(path, ttcIndex);
}

SkTypeface* SkFontMgr_Indirect::onCreateFromData(SkData* data, int ttcIndex) const {
    return fImpl->createFromData(data, ttcIndex);
}

SkTypeface* SkFontMgr_Indirect::onLegacyCreateTypeface(const char familyName[],
                                                       unsigned styleBits) const {
    bool bold = SkToBool(styleBits & SkTypeface::kBold);
    bool italic = SkToBool(styleBits & SkTypeface::kItalic);
    SkFontStyle style = SkFontStyle(bold ? SkFontStyle::kBold_Weight
                                         : SkFontStyle::kNormal_Weight,
                                    SkFontStyle::kNormal_Width,
                                    italic ? SkFontStyle::kItalic_Slant
                                           : SkFontStyle::kUpright_Slant);

    SkAutoTUnref<SkTypeface> face(this->matchFamilyStyle(familyName, style));

    if (NULL == face.get()) {
        face.reset(this->matchFamilyStyle(NULL, style));
    }

    if (NULL == face.get()) {
        SkFontIdentity fontId = this->fProxy->matchIndexStyle(0, style);
        face.reset(this->createTypefaceFromFontId(fontId));
    }

    return face.detach();
}
