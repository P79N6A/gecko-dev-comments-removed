















#ifndef ESDS_H_

#define ESDS_H_

#include <stdint.h>

#include <media/stagefright/MediaErrors.h>

namespace stagefright {

class ESDS {
public:
    ESDS(const void *data, size_t size);
    ~ESDS();

    status_t InitCheck() const;

    status_t getObjectTypeIndication(uint8_t *objectTypeIndication) const;
    status_t getCodecSpecificInfo(const void **data, size_t *size) const;
    status_t getCodecSpecificOffset(size_t *offset, size_t *size) const;
    status_t getBitRate(uint32_t *brateMax, uint32_t *brateAvg) const;
    status_t getStreamType(uint8_t *streamType) const;

private:
    enum {
        kTag_ESDescriptor            = 0x03,
        kTag_DecoderConfigDescriptor = 0x04,
        kTag_DecoderSpecificInfo     = 0x05
    };

    uint8_t *mData;
    size_t mSize;

    status_t mInitCheck;

    size_t mDecoderSpecificOffset;
    size_t mDecoderSpecificLength;
    uint8_t mObjectTypeIndication;
    uint8_t mStreamType;
    uint32_t mBitRateMax;
    uint32_t mBitRateAvg;

    status_t skipDescriptorHeader(
            size_t offset, size_t size,
            uint8_t *tag, size_t *data_offset, size_t *data_size) const;

    status_t parse();
    status_t parseESDescriptor(size_t offset, size_t size);
    status_t parseDecoderConfigDescriptor(size_t offset, size_t size);

    ESDS(const ESDS &);
    ESDS &operator=(const ESDS &);
};

}  
#endif
