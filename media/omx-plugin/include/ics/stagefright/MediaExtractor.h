















#ifndef MEDIA_EXTRACTOR_H_

#define MEDIA_EXTRACTOR_H_

#include <utils/RefBase.h>

namespace android {

class DataSource;
class MediaSource;
class MetaData;

class MediaExtractor : public RefBase {
public:
    static sp<MediaExtractor> Create(
            const sp<DataSource> &source, const char *mime = NULL);

    virtual size_t countTracks() = 0;
    virtual sp<MediaSource> getTrack(size_t index) = 0;

    enum GetTrackMetaDataFlags {
        kIncludeExtensiveMetaData = 1
    };
    virtual sp<MetaData> getTrackMetaData(
            size_t index, uint32_t flags = 0) = 0;

    
    
    virtual sp<MetaData> getMetaData();

    enum Flags {
        CAN_SEEK_BACKWARD  = 1,  
        CAN_SEEK_FORWARD   = 2,  
        CAN_PAUSE          = 4,
        CAN_SEEK           = 8,  
    };

    
    
    virtual uint32_t flags() const;

    
    virtual void setDrmFlag(bool flag) {
        mIsDrm = flag;
    };
    virtual bool getDrmFlag() {
        return mIsDrm;
    }
    virtual char* getDrmTrackInfo(size_t trackID, int *len) {
        return NULL;
    }

protected:
    MediaExtractor() {}
    virtual ~MediaExtractor() {}

private:
    bool mIsDrm;

    MediaExtractor(const MediaExtractor &);
    MediaExtractor &operator=(const MediaExtractor &);
};

}  

#endif  
