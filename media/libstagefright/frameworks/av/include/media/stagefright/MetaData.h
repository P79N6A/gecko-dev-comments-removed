















#ifndef META_DATA_H_

#define META_DATA_H_

#include <sys/types.h>

#include <stdint.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace stagefright {


enum {
    kKeyMIMEType          = 'mime',  
    kKeyWidth             = 'widt',  
    kKeyHeight            = 'heig',  
    kKeyDisplayWidth      = 'dWid',  
    kKeyDisplayHeight     = 'dHgt',  
    kKeySARWidth          = 'sarW',  
    kKeySARHeight         = 'sarH',  

    
    kKeyCropRect          = 'crop',

    kKeyRotation          = 'rotA',  
    kKeyIFramesInterval   = 'ifiv',  
    kKeyStride            = 'strd',  
    kKeySliceHeight       = 'slht',  
    kKeyChannelCount      = '#chn',  
    kKeyChannelMask       = 'chnm',  
    kKeySampleRate        = 'srte',  
    kKeySampleSize        = 'ssiz',  
    kKeyFrameRate         = 'frmR',  
    kKeyBitRate           = 'brte',  
    kKeyESDS              = 'esds',  
    kKeyAACProfile        = 'aacp',  
    kKeyAVCC              = 'avcc',  
    kKeyD263              = 'd263',  
    kKeyVorbisInfo        = 'vinf',  
    kKeyVorbisBooks       = 'vboo',  
    kKeyWantsNALFragments = 'NALf',
    kKeyIsSyncFrame       = 'sync',  
    kKeyIsCodecConfig     = 'conf',  
    kKeyTime              = 'time',  
    kKeyDecodingTime      = 'decT',  
    kKeyNTPTime           = 'ntpT',  
    kKeyTargetTime        = 'tarT',  
    kKeyDriftTime         = 'dftT',  
    kKeyAnchorTime        = 'ancT',  
    kKeyDuration          = 'dura',  
    kKeyMovieDuration     = 'mdur',  
    kKeyColorFormat       = 'colf',
    kKeyPlatformPrivate   = 'priv',  
    kKeyDecoderComponent  = 'decC',  
    kKeyBufferID          = 'bfID',
    kKeyMaxInputSize      = 'inpS',
    kKeyThumbnailTime     = 'thbT',  
    kKeyTrackID           = 'trID',
    kKeyIsDRM             = 'idrm',  
    kKeyEncoderDelay      = 'encd',  
    kKeyEncoderPadding    = 'encp',  
    kKeyMediaTime         = 'mtme',  

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
    kKeyCompilation       = 'cpil',  
    kKeyLocation          = 'loc ',  
    kKeyTimeScale         = 'tmsl',  

    
    kKeyVideoProfile      = 'vprf',  
    kKeyVideoLevel        = 'vlev',  

    
    kKey64BitFileOffset   = 'fobt',  
    kKey2ByteNalLength    = '2NAL',  

    
    
    
    kKeyFileType          = 'ftyp',  

    
    
    kKeyTrackTimeStatus   = 'tktm',  

    kKeyRealTimeRecording = 'rtrc',  
    kKeyNumBuffers        = 'nbbf',  

    
    kKeyAutoLoop          = 'autL',  

    kKeyValidSamples      = 'valD',  

    kKeyIsUnreadable      = 'unre',  

    
    kKeyRendered          = 'rend',  

    
    kKeyMediaLanguage     = 'lang',  

    
    kKeyTextFormatData    = 'text',  

    kKeyRequiresSecureBuffers = 'secu',  

    kKeyIsADTS            = 'adts',  

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    kKeyEncryptedSizes    = 'encr',  
    kKeyPlainSizes        = 'plai',  
    kKeyCryptoKey         = 'cryK',  
    kKeyCryptoIV          = 'cryI',  
    kKeyCryptoMode        = 'cryM',  

    kKeyCryptoDefaultIVSize = 'cryS',  

    kKeyPssh              = 'pssh',  
};

enum {
    kTypeESDS        = 'esds',
    kTypeAVCC        = 'avcc',
    kTypeD263        = 'd263',
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
        TYPE_RECT     = 'rect',
    };

    void clear();
    bool remove(uint32_t key);

    bool setCString(uint32_t key, const char *value);
    bool setInt32(uint32_t key, int32_t value);
    bool setInt64(uint32_t key, int64_t value);
    bool setFloat(uint32_t key, float value);
    bool setPointer(uint32_t key, void *value);

    bool setRect(
            uint32_t key,
            int32_t left, int32_t top,
            int32_t right, int32_t bottom);

    bool findCString(uint32_t key, const char **value) const;
    bool findInt32(uint32_t key, int32_t *value) const;
    bool findInt64(uint32_t key, int64_t *value) const;
    bool findFloat(uint32_t key, float *value) const;
    bool findPointer(uint32_t key, void **value) const;

    bool findRect(
            uint32_t key,
            int32_t *left, int32_t *top,
            int32_t *right, int32_t *bottom) const;

    bool setData(uint32_t key, uint32_t type, const void *data, size_t size);

    bool findData(uint32_t key, uint32_t *type,
                  const void **data, size_t *size) const;

    void dumpToLog() const;

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
        String8 asString() const;

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

    struct Rect {
        int32_t mLeft, mTop, mRight, mBottom;
    };

    KeyedVector<uint32_t, typed_data> mItems;

    
};

}  

#endif  
