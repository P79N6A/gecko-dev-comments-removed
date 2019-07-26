















#ifndef A_SESSION_DESCRIPTION_H_

#define A_SESSION_DESCRIPTION_H_

#include "mozilla/Types.h"
#include <sys/types.h>

#include <media/stagefright/foundation/ABase.h>
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>

namespace android {

struct MOZ_EXPORT AString;

struct ASessionDescription : public RefBase {
    ASessionDescription();

    bool setTo(const void *data, size_t size);
    bool isValid() const;

    
    
    size_t countTracks() const;
    void getFormat(size_t index, AString *value) const;

    bool getFormatType(
            size_t index, unsigned long *PT,
            AString *desc, AString *params) const;

    bool getDimensions(
            size_t index, unsigned long PT,
            int32_t *width, int32_t *height) const;

    bool getDurationUs(int64_t *durationUs) const;

    static void ParseFormatDesc(
            const char *desc, int32_t *timescale, int32_t *numChannels);

    bool findAttribute(size_t index, const char *key, AString *value) const;

    
    
    
    
    
    
    static bool parseNTPRange(const char *s, float *npt1, float *npt2);

protected:
    virtual ~ASessionDescription();

private:
    typedef KeyedVector<AString,AString> Attribs;

    bool mIsValid;
    Vector<Attribs> mTracks;
    Vector<AString> mFormats;

    bool parse(const void *data, size_t size);

    DISALLOW_EVIL_CONSTRUCTORS(ASessionDescription);
};

}  

#endif  
