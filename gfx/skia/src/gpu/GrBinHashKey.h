








#ifndef GrBinHashKey_DEFINED
#define GrBinHashKey_DEFINED

#include "GrTypes.h"












template<typename ENTRY, size_t KEY_SIZE>
class GrTBinHashKey {
public:
    enum { kKeySize = KEY_SIZE };

    GrTBinHashKey() {
        this->reset();
    }

    GrTBinHashKey(const GrTBinHashKey<ENTRY, KEY_SIZE>& other) {
        *this = other;
    }

    GrTBinHashKey<ENTRY, KEY_SIZE>& operator=(const GrTBinHashKey<ENTRY, KEY_SIZE>& other) {
        memcpy(this, &other, sizeof(*this));
        return *this;
    }

    ~GrTBinHashKey() {
    }

    void reset() {
        fHash = 0;
#if GR_DEBUG
        fIsValid = false;
#endif
    }

    void setKeyData(const uint32_t* SK_RESTRICT data) {
        GrAssert(GrIsALIGN4(KEY_SIZE));
        memcpy(&fData, data, KEY_SIZE);

        uint32_t hash = 0;
        size_t len = KEY_SIZE;
        while (len >= 4) {
            hash += *data++;
            hash += (fHash << 10);
            hash ^= (hash >> 6);
            len -= 4;
        }
        hash += (fHash << 3);
        hash ^= (fHash >> 11);
        hash += (fHash << 15);
#if GR_DEBUG
        fIsValid = true;
#endif
        fHash = hash;
    }

    int compare(const GrTBinHashKey<ENTRY, KEY_SIZE>& key) const {
        GrAssert(fIsValid && key.fIsValid);
        return memcmp(fData, key.fData, KEY_SIZE);
    }

    static bool EQ(const ENTRY& entry, const GrTBinHashKey<ENTRY, KEY_SIZE>& key) {
        GrAssert(key.fIsValid);
        return 0 == entry.compare(key);
    }

    static bool LT(const ENTRY& entry, const GrTBinHashKey<ENTRY, KEY_SIZE>& key) {
        GrAssert(key.fIsValid);
        return entry.compare(key) < 0;
    }

    uint32_t getHash() const {
        GrAssert(fIsValid);
        return fHash;
    }

    const uint8_t* getData() const {
        GrAssert(fIsValid);
        return fData;
    }

private:
    uint32_t            fHash;
    uint8_t             fData[KEY_SIZE];  

#if GR_DEBUG
public:
    bool                fIsValid;
#endif
};

#endif
