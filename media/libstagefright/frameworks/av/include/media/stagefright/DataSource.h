















#ifndef DATA_SOURCE_H_

#define DATA_SOURCE_H_

#include <sys/types.h>

#include <media/stagefright/MediaErrors.h>
#include <utils/RefBase.h>

namespace android {

class String8;

class DataSource : public RefBase {
public:
    enum Flags {
        kWantsPrefetching      = 1,
        kStreamedFromLocalHost = 2,
        kIsCachingDataSource   = 4,
        kIsHTTPBasedSource     = 8,
    };

    DataSource() {}

    virtual status_t initCheck() const = 0;

    virtual ssize_t readAt(off64_t offset, void *data, size_t size) = 0;

    
    bool getUInt16(off64_t offset, uint16_t *x);
    bool getUInt24(off64_t offset, uint32_t *x); 
    bool getUInt32(off64_t offset, uint32_t *x);
    bool getUInt64(off64_t offset, uint64_t *x);

    
    virtual status_t getSize(off64_t *size);

    virtual uint32_t flags() {
        return 0;
    }

    virtual status_t reconnectAtOffset(off64_t offset) {
        return ERROR_UNSUPPORTED;
    }

#if 0
    

    bool sniff(String8 *mimeType, float *confidence, sp<AMessage> *meta);

    
    
    
    typedef bool (*SnifferFunc)(
            const sp<DataSource> &source, String8 *mimeType,
            float *confidence, sp<AMessage> *meta);

    static void RegisterSniffer(SnifferFunc func);
    static void RegisterDefaultSniffers();

    
    virtual sp<DecryptHandle> DrmInitialization(const char *mime = NULL) {
        return NULL;
    }
    virtual void getDrmInfo(sp<DecryptHandle> &handle, DrmManagerClient **client) {};

    virtual String8 getUri() {
        return String8();
    }
#endif

    virtual String8 getMIMEType() const;

protected:
    virtual ~DataSource() {}

private:
    DataSource(const DataSource &);
    DataSource &operator=(const DataSource &);
};

}  

#endif
