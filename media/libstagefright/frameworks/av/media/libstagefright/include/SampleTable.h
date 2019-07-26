















#ifndef SAMPLE_TABLE_H_

#define SAMPLE_TABLE_H_

#include <sys/types.h>
#include <stdint.h>

#include <media/stagefright/MediaErrors.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

namespace android {

class DataSource;
struct SampleIterator;

class SampleTable : public RefBase {
public:
    SampleTable(const sp<DataSource> &source);

    bool isValid() const;

    
    status_t setChunkOffsetParams(
            uint32_t type, off64_t data_offset, size_t data_size);

    status_t setSampleToChunkParams(off64_t data_offset, size_t data_size);

    
    status_t setSampleSizeParams(
            uint32_t type, off64_t data_offset, size_t data_size);

    status_t setTimeToSampleParams(off64_t data_offset, size_t data_size);

    status_t setCompositionTimeToSampleParams(
            off64_t data_offset, size_t data_size);

    status_t setSyncSampleParams(off64_t data_offset, size_t data_size);

    

    uint32_t countChunkOffsets() const;

    uint32_t countSamples() const;

    status_t getMaxSampleSize(size_t *size);

    status_t getMetaDataForSample(
            uint32_t sampleIndex,
            off64_t *offset,
            size_t *size,
            uint32_t *compositionTime,
            bool *isSyncSample = NULL);

    enum {
        kFlagBefore,
        kFlagAfter,
        kFlagClosest
    };
    status_t findSampleAtTime(
            uint32_t req_time, uint32_t *sample_index, uint32_t flags);

    status_t findSyncSampleNear(
            uint32_t start_sample_index, uint32_t *sample_index,
            uint32_t flags);

    status_t findThumbnailSample(uint32_t *sample_index);

protected:
    ~SampleTable();

private:
    struct CompositionDeltaLookup;

    static const uint32_t kChunkOffsetType32;
    static const uint32_t kChunkOffsetType64;
    static const uint32_t kSampleSizeType32;
    static const uint32_t kSampleSizeTypeCompact;

    sp<DataSource> mDataSource;
    Mutex mLock;

    off64_t mChunkOffsetOffset;
    uint32_t mChunkOffsetType;
    uint32_t mNumChunkOffsets;

    off64_t mSampleToChunkOffset;
    uint32_t mNumSampleToChunkOffsets;

    off64_t mSampleSizeOffset;
    uint32_t mSampleSizeFieldSize;
    uint32_t mDefaultSampleSize;
    uint32_t mNumSampleSizes;

    uint32_t mTimeToSampleCount;
    uint32_t *mTimeToSample;

    struct SampleTimeEntry {
        uint32_t mSampleIndex;
        uint32_t mCompositionTime;
    };
    SampleTimeEntry *mSampleTimeEntries;

    uint32_t *mCompositionTimeDeltaEntries;
    size_t mNumCompositionTimeDeltaEntries;
    CompositionDeltaLookup *mCompositionDeltaLookup;

    off64_t mSyncSampleOffset;
    uint32_t mNumSyncSamples;
    uint32_t *mSyncSamples;
    size_t mLastSyncSampleIndex;

    SampleIterator *mSampleIterator;

    struct SampleToChunkEntry {
        uint32_t startChunk;
        uint32_t samplesPerChunk;
        uint32_t chunkDesc;
    };
    SampleToChunkEntry *mSampleToChunkEntries;

    friend struct SampleIterator;

    status_t getSampleSize_l(uint32_t sample_index, size_t *sample_size);
    uint32_t getCompositionTimeOffset(uint32_t sampleIndex);

    static int CompareIncreasingTime(const void *, const void *);

    void buildSampleEntriesTable();

    SampleTable(const SampleTable &);
    SampleTable &operator=(const SampleTable &);
};

}  

#endif  
