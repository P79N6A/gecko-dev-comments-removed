















#ifndef META_DATA_H_

#define META_DATA_H_

#include <sys/types.h>

#include <stdint.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>

namespace android {


enum {
    kKeyMIMEType          = 'mime',  
    kKeyWidth             = 'widt',
    kKeyHeight            = 'heig',
    kKeyChannelCount      = '#chn',
    kKeySampleRate        = 'srte',
    kKeyBitRate           = 'brte',  
    kKeyESDS              = 'esds',  
    kKeyAVCC              = 'avcc',  
    kKeyVorbisInfo        = 'vinf',  
    kKeyVorbisBooks       = 'vboo',  
    kKeyWantsNALFragments = 'NALf',
    kKeyIsSyncFrame       = 'sync',  
    kKeyIsCodecConfig     = 'conf',  
    kKeyTime              = 'time',  
    kKeyDuration          = 'dura',  
    kKeyColorFormat       = 'colf',
    kKeyPlatformPrivate   = 'priv',  
    kKeyDecoderComponent  = 'decC',  
    kKeyBufferID          = 'bfID',
    kKeyMaxInputSize      = 'inpS',
    kKeyThumbnailTime     = 'thbT',  

    kKeyAlbum             = 'albu',  
    kKeyArtist            = 'arti',  
    kKeyAlbumArtist       = 'aart',  
    kKeyComposer          = 'comp',  
    kKeyGenre             = 'genr',  
    kKeyTitle             = 'titl',  
    kKeyYear              = 'year',  
    kKeyAlbumArt          = 'albA',  
    kKeyAlbumArtMIME      = 'alAM',  
    kKeyAuthor            = 'auth',  
    kKeyCDTrackNumber     = 'cdtr',  
    kKeyDiscNumber        = 'dnum',  
    kKeyDate              = 'date',  
    kKeyWriter            = 'writ',  
};

enum {
    kTypeESDS        = 'esds',
    kTypeAVCC        = 'avcc',
};

class MetaData : public RefBase {
public:
    MetaData();
    MetaData(const MetaData &from);

    enum Type {
        TYPE_NONE     = 'none',
        TYPE_C_STRING = 'cstr',
        TYPE_INT32    = 'in32',
        TYPE_INT64    = 'in64',
        TYPE_FLOAT    = 'floa',
        TYPE_POINTER  = 'ptr ',
    };

    void clear();
    bool remove(uint32_t key);

    bool setCString(uint32_t key, const char *value);
    bool setInt32(uint32_t key, int32_t value);
    bool setInt64(uint32_t key, int64_t value);
    bool setFloat(uint32_t key, float value);
    bool setPointer(uint32_t key, void *value);

    bool findCString(uint32_t key, const char **value);
    bool findInt32(uint32_t key, int32_t *value);
    bool findInt64(uint32_t key, int64_t *value);
    bool findFloat(uint32_t key, float *value);
    bool findPointer(uint32_t key, void **value);

    bool setData(uint32_t key, uint32_t type, const void *data, size_t size);

    bool findData(uint32_t key, uint32_t *type,
                  const void **data, size_t *size) const;

protected:
    virtual ~MetaData();

private:
    struct typed_data {
        typed_data();
        ~typed_data();

        typed_data(const MetaData::typed_data &);
        typed_data &operator=(const MetaData::typed_data &);

        void clear();
        void setData(uint32_t type, const void *data, size_t size);
        void getData(uint32_t *type, const void **data, size_t *size) const;

    private:
        uint32_t mType;
        size_t mSize;

        union {
            void *ext_data;
            float reservoir;
        } u;

        bool usesReservoir() const {
            return mSize <= sizeof(u.reservoir);
        }

        void allocateStorage(size_t size);
        void freeStorage();

        void *storage() {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }

        const void *storage() const {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }
    };

    KeyedVector<uint32_t, typed_data> mItems;

    
};

}  

#endif  
