















#ifndef DATA_SOURCE_H_

#define DATA_SOURCE_H_

#include <sys/types.h>

#include <media/stagefright/MediaErrors.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <drm/DrmManagerClient.h>

#if !defined(STAGEFRIGHT_EXPORT)
#define STAGEFRIGHT_EXPORT
#endif

namespace android {

struct AMessage;
class String8;

class STAGEFRIGHT_EXPORT DataSource : public RefBase {
public:
    enum Flags {
        kWantsPrefetching      = 1,
        kStreamedFromLocalHost = 2,
        kIsCachingDataSource   = 4,
        kIsHTTPBasedSource     = 8,
    };

    static sp<DataSource> CreateFromURI(
            const char *uri,
            const KeyedVector<String8, String8> *headers = NULL);

    DataSource() {}

    virtual status_t initCheck() const = 0;

    virtual ssize_t readAt(off64_t offset, void *data, size_t size) = 0;

    
    bool getUInt16(off64_t offset, uint16_t *x);

    
    virtual status_t getSize(off64_t *size);

    virtual uint32_t flags() {
        return 0;
    }

    virtual status_t reconnectAtOffset(off64_t offset) {
        return ERROR_UNSUPPORTED;
    }

    

    bool sniff(String8 *mimeType, float *confidence, sp<AMessage> *meta);

    
    
    
    typedef bool (*SnifferFunc)(
            const sp<DataSource> &source, String8 *mimeType,
            float *confidence, sp<AMessage> *meta);

    static void RegisterSniffer(SnifferFunc func);
    static void RegisterDefaultSniffers();

    
    virtual sp<DecryptHandle> DrmInitialization() {
        return NULL;
    }
    virtual void getDrmInfo(sp<DecryptHandle> &handle, DrmManagerClient **client) {};

    virtual String8 getUri() {
        return String8();
    }

    virtual String8 getMIMEType() const;

protected:
    virtual ~DataSource() {}

private:
    static Mutex gSnifferMutex;
    static List<SnifferFunc> gSniffers;

    DataSource(const DataSource &);
    DataSource &operator=(const DataSource &);
};

}  

#endif
