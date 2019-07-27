















#ifndef MPEG4_EXTRACTOR_H_

#define MPEG4_EXTRACTOR_H_

#include <arpa/inet.h>

#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/Utils.h>
#include <utils/List.h>
#include <utils/Vector.h>
#include <utils/String8.h>

namespace stagefright {

struct AMessage;
class DataSource;
class SampleTable;
class String8;

struct SidxEntry {
    size_t mSize;
    uint32_t mDurationUs;
};

class MPEG4Extractor : public MediaExtractor {
public:
    
    MPEG4Extractor(const sp<DataSource> &source);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();
    virtual uint32_t flags() const;

    
    virtual char* getDrmTrackInfo(size_t trackID, int *len);

    struct TrackExtends {
        TrackExtends(): mVersion(0), mTrackId(0),
                mDefaultSampleDescriptionIndex(0), mDefaultSampleDuration(0),
                mDefaultSampleSize(0), mDefaultSampleFlags(0)
        {
            mFlags[0] = 0;
            mFlags[1] = 0;
            mFlags[2] = 0;
        }
        uint8_t mVersion;
        uint8_t mFlags[3];
        uint32_t mTrackId;
        uint32_t mDefaultSampleDescriptionIndex;
        uint32_t mDefaultSampleDuration;
        uint32_t mDefaultSampleSize;
        uint32_t mDefaultSampleFlags;
    };

protected:
    virtual ~MPEG4Extractor();

private:

    struct PsshInfo {
        uint8_t uuid[16];
        uint32_t datalen;
        uint8_t *data;
    };
    struct Track {
        Track *next;
        sp<MetaData> meta;
        uint32_t timescale;
        
        
        uint64_t empty_duration;
        uint64_t segment_duration;
        int64_t media_time;

        sp<SampleTable> sampleTable;
        bool includes_expensive_metadata;
        bool skipTrack;
    };

    Vector<SidxEntry> mSidxEntries;
    uint64_t mSidxDuration;

    Vector<PsshInfo> mPssh;

    sp<DataSource> mDataSource;
    status_t mInitCheck;
    bool mHasVideo;
    uint32_t mHeaderTimescale;

    Track *mFirstTrack, *mLastTrack;

    sp<MetaData> mFileMetaData;

    Vector<uint32_t> mPath;
    String8 mLastCommentMean;
    String8 mLastCommentName;
    String8 mLastCommentData;

    status_t readMetaData();
    status_t parseChunk(off64_t *offset, int depth);
    status_t parseMetaData(off64_t offset, size_t size);

    status_t updateAudioTrackInfoFromESDS_MPEG4Audio(
            const void *esds_data, size_t esds_size);

    static status_t verifyTrack(Track *track);

    struct SINF {
        SINF *next;
        uint16_t trackID;
        uint8_t IPMPDescriptorID;
        ssize_t len;
        char *IPMPData;
    };

    SINF *mFirstSINF;

    bool mIsDrm;
    TrackExtends mTrackExtends;
    uint32_t mDrmScheme;

    status_t parseDrmSINF(off64_t *offset, off64_t data_offset);

    status_t parseTrackExtends(off64_t data_offset, off64_t data_size);

    status_t parseTrackHeader(off64_t data_offset, off64_t data_size);

    status_t parseSegmentIndex(off64_t data_offset, size_t data_size);

    void storeEditList();

    Track *findTrackByMimePrefix(const char *mimePrefix);

    MPEG4Extractor(const MPEG4Extractor &);
    MPEG4Extractor &operator=(const MPEG4Extractor &);
};

bool SniffMPEG4(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  

#endif  
