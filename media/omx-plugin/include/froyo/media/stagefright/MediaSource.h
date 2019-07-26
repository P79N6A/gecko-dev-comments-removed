















#ifndef MEDIA_SOURCE_H_

#define MEDIA_SOURCE_H_

#include <sys/types.h>

#include <utils/RefBase.h>

namespace android {

class MediaBuffer;
class MetaData;

struct MediaSource : public RefBase {
    MediaSource();

    
    
    virtual status_t start(MetaData *params = NULL) = 0;

    
    
    
    
    
    
    virtual status_t stop() = 0;

    
    virtual sp<MetaData> getFormat() = 0;

    struct ReadOptions;

    
    
    
    
    
    
    
    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL) = 0;

    
    
    
    struct ReadOptions {
        ReadOptions();

        
        void reset();

        void setSeekTo(int64_t time_us);
        void clearSeekTo();
        bool getSeekTo(int64_t *time_us) const;

        void setLateBy(int64_t lateness_us);
        int64_t getLateBy() const;

    private:
        enum Options {
            kSeekTo_Option      = 1,
        };

        uint32_t mOptions;
        int64_t mSeekTimeUs;
        int64_t mLatenessUs;
    };

protected:
    virtual ~MediaSource();

private:
    MediaSource(const MediaSource &);
    MediaSource &operator=(const MediaSource &);
};

}  

#endif  
