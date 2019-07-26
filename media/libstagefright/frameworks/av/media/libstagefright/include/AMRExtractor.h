















#ifndef AMR_EXTRACTOR_H_

#define AMR_EXTRACTOR_H_

#include <utils/Errors.h>
#include <media/stagefright/MediaExtractor.h>

namespace stagefright {

struct AMessage;
class String8;
#define OFFSET_TABLE_LEN    300

class AMRExtractor : public MediaExtractor {
public:
    AMRExtractor(const sp<DataSource> &source);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

protected:
    virtual ~AMRExtractor();

private:
    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;
    status_t mInitCheck;
    bool mIsWide;

    off64_t mOffsetTable[OFFSET_TABLE_LEN]; 
    size_t mOffsetTableLength;

    AMRExtractor(const AMRExtractor &);
    AMRExtractor &operator=(const AMRExtractor &);
};

bool SniffAMR(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  

#endif  
