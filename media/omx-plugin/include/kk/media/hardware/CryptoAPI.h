















#include <utils/Errors.h>

#ifndef CRYPTO_API_H_

#define CRYPTO_API_H_

namespace android {

struct AString;
struct CryptoPlugin;

struct CryptoFactory {
    CryptoFactory() {}
    virtual ~CryptoFactory() {}

    virtual bool isCryptoSchemeSupported(const uint8_t uuid[16]) const = 0;

    virtual status_t createPlugin(
            const uint8_t uuid[16], const void *data, size_t size,
            CryptoPlugin **plugin) = 0;

private:
    CryptoFactory(const CryptoFactory &);
    CryptoFactory &operator=(const CryptoFactory &);
};

struct CryptoPlugin {
    enum Mode {
        kMode_Unencrypted = 0,
        kMode_AES_CTR     = 1,

        
        
        kMode_AES_WV      = 2,  
    };

    struct SubSample {
        size_t mNumBytesOfClearData;
        size_t mNumBytesOfEncryptedData;
    };

    CryptoPlugin() {}
    virtual ~CryptoPlugin() {}

    
    
    
    
    virtual bool requiresSecureDecoderComponent(const char *mime) const = 0;

    
    
    
    
    
    
    
    
    virtual ssize_t decrypt(
            bool secure,
            const uint8_t key[16],
            const uint8_t iv[16],
            Mode mode,
            const void *srcPtr,
            const SubSample *subSamples, size_t numSubSamples,
            void *dstPtr,
            AString *errorDetailMsg) = 0;

private:
    CryptoPlugin(const CryptoPlugin &);
    CryptoPlugin &operator=(const CryptoPlugin &);
};

}  

extern "C" {
    extern android::CryptoFactory *createCryptoFactory();
}

#endif  
