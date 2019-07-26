















#ifndef MEDIA_SOURCE_H_

#define MEDIA_SOURCE_H_

#include <sys/types.h>

#include <media/stagefright/MediaErrors.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>

namespace stagefright {

class MediaBuffer;
class MetaData;

struct MediaSource : public virtual RefBase {
    MediaSource();

    
    
    virtual status_t start(MetaData *params = NULL) = 0;

    
    
    
    
    
    
    virtual status_t stop() = 0;

    
    virtual sp<MetaData> getFormat() = 0;

    struct ReadOptions;

    
    
    
    
    
    
    
    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL) = 0;

    
    
    
    struct ReadOptions {
        enum SeekMode {
            SEEK_PREVIOUS_SYNC,
            SEEK_NEXT_SYNC,
            SEEK_CLOSEST_SYNC,
            SEEK_CLOSEST,
        };

        ReadOptions();

        
        void reset();

        void setSeekTo(int64_t time_us, SeekMode mode = SEEK_CLOSEST_SYNC);
        void clearSeekTo();
        bool getSeekTo(int64_t *time_us, SeekMode *mode) const;

        void setLateBy(int64_t lateness_us);
        int64_t getLateBy() const;

    private:
        enum Options {
            kSeekTo_Option      = 1,
        };

        uint32_t mOptions;
        int64_t mSeekTimeUs;
        SeekMode mSeekMode;
        int64_t mLatenessUs;
    };

    
    
    
    virtual status_t pause() {
        return ERROR_UNSUPPORTED;
    }

    
    
    
    
    
    virtual status_t setBuffers(const Vector<MediaBuffer *> &buffers) {
        return ERROR_UNSUPPORTED;
    }

protected:
    virtual ~MediaSource();

private:
    MediaSource(const MediaSource &);
    MediaSource &operator=(const MediaSource &);
};

}  

#endif  
