















#define LOG_TAG "ASessionDescription"
#include "RtspPrlog.h"

#include "ASessionDescription.h"

#include "mozilla/Snprintf.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>

#include <stdlib.h>

namespace android {

ASessionDescription::ASessionDescription()
    : mIsValid(false) {
}

ASessionDescription::~ASessionDescription() {
}

bool ASessionDescription::setTo(const void *data, size_t size) {
    mIsValid = parse(data, size);

    if (!mIsValid) {
        mTracks.clear();
        mFormats.clear();
    }

    return mIsValid;
}

bool ASessionDescription::parse(const void *data, size_t size) {
    mTracks.clear();
    mFormats.clear();

    mTracks.push(Attribs());
    mFormats.push(AString("[root]"));

    AString desc((const char *)data, size);

    size_t i = 0;
    for (;;) {
        ssize_t eolPos = desc.find("\n", i);

        if (eolPos < 0) {
            break;
        }

        AString line;
        if ((size_t)eolPos > i && desc.c_str()[eolPos - 1] == '\r') {
            
            
            line.setTo(desc, i, eolPos - i - 1);
        } else {
            line.setTo(desc, i, eolPos - i);
        }

        if (line.empty()) {
            i = eolPos + 1;
            continue;
        }

        if (line.size() < 2 || line.c_str()[1] != '=') {
            return false;
        }

        LOGI("%s", line.c_str());

        switch (line.c_str()[0]) {
            case 'v':
            {
                if (strcmp(line.c_str(), "v=0")) {
                    return false;
                }
                break;
            }

            case 'a':
            case 'b':
            {
                AString key, value;

                ssize_t colonPos = line.find(":", 2);
                if (colonPos < 0) {
                    key = line;
                } else {
                    key.setTo(line, 0, colonPos);

                    if (key == "a=fmtp" || key == "a=rtpmap"
                            || key == "a=framesize") {
                        ssize_t spacePos = line.find(" ", colonPos + 1);
                        if (spacePos < 0) {
                            return false;
                        }

                        key.setTo(line, 0, spacePos);

                        colonPos = spacePos;
                    }

                    value.setTo(line, colonPos + 1, line.size() - colonPos - 1);
                }

                key.trim();
                value.trim();

                LOGV("adding '%s' => '%s'", key.c_str(), value.c_str());

                mTracks.editItemAt(mTracks.size() - 1).add(key, value);
                break;
            }

            case 'm':
            {
                LOGV("new section '%s'",
                     AString(line, 2, line.size() - 2).c_str());

                mTracks.push(Attribs());
                mFormats.push(AString(line, 2, line.size() - 2));
                break;
            }

            default:
            {
                AString key, value;

                ssize_t equalPos = line.find("=");

                key = AString(line, 0, equalPos + 1);
                value = AString(line, equalPos + 1, line.size() - equalPos - 1);

                key.trim();
                value.trim();

                LOGV("adding '%s' => '%s'", key.c_str(), value.c_str());

                mTracks.editItemAt(mTracks.size() - 1).add(key, value);
                break;
            }
        }

        i = eolPos + 1;
    }

    return true;
}

bool ASessionDescription::isValid() const {
    return mIsValid;
}

size_t ASessionDescription::countTracks() const {
    return mTracks.size();
}

void ASessionDescription::getFormat(size_t index, AString *value) const {
    CHECK_GE(index, 0u);
    CHECK_LT(index, mTracks.size());

    *value = mFormats.itemAt(index);
}

bool ASessionDescription::findAttribute(
        size_t index, const char *key, AString *value) const {
    CHECK_GE(index, 0u);
    CHECK_LT(index, mTracks.size());

    value->clear();

    const Attribs &track = mTracks.itemAt(index);
    ssize_t i = track.indexOfKey(AString(key));

    if (i < 0) {
        return false;
    }

    *value = track.valueAt(i);

    return true;
}

bool ASessionDescription::getFormatType(
        size_t index, unsigned long *PT,
        AString *desc, AString *params) const {
    AString format;
    getFormat(index, &format);

    const char *lastSpacePos = strrchr(format.c_str(), ' ');
    if (!lastSpacePos) {
        return false;
    }

    char *end;
    unsigned long x = strtoul(lastSpacePos + 1, &end, 10);
    if (end <= lastSpacePos + 1 || *end != '\0') {
        return false;
    }

    *PT = x;

    char key[20];
    snprintf_literal(key, "a=rtpmap:%lu", x);

    if (!findAttribute(index, key, desc)) {
        
        
        
        return false;
    }

    snprintf_literal(key, "a=fmtp:%lu", x);
    if (!findAttribute(index, key, params)) {
        params->clear();
    }

    return true;
}

bool ASessionDescription::getDimensions(
        size_t index, unsigned long PT,
        int32_t *width, int32_t *height) const {
    *width = 0;
    *height = 0;

    char key[20];
    snprintf_literal(key, "a=framesize:%lu", PT);
    AString value;
    if (!findAttribute(index, key, &value)) {
        return false;
    }

    const char *s = value.c_str();
    char *end;
    *width = strtoul(s, &end, 10);
    if (end <= s || *end != '-') {
        return false;
    }

    s = end + 1;
    *height = strtoul(s, &end, 10);
    if (end <= s || *end != '\0') {
        return false;
    }

    return true;
}

bool ASessionDescription::getDurationUs(int64_t *durationUs) const {
    *durationUs = 0;

    if (!mIsValid) {
        return false;
    }

    AString value;
    if (!findAttribute(0, "a=range", &value)) {
        return false;
    }

    if (strncmp(value.c_str(), "npt=", 4)) {
        return false;
    }

    float from, to;
    if (!parseNTPRange(value.c_str() + 4, &from, &to)) {
        return false;
    }

    *durationUs = (int64_t)((to - from) * 1E6);

    return true;
}


bool ASessionDescription::ParseFormatDesc(
        const char *desc, int32_t *timescale, int32_t *numChannels) {
    const char *slash1 = strchr(desc, '/');
    if (!slash1) {
        return false;
    }

    const char *s = slash1 + 1;
    char *end;
    unsigned long x = strtoul(s, &end, 10);
    if (end <= s) {
        return false;
    }
    if (*end != '\0' && *end != '/') {
        return false;
    }

    *timescale = x;
    *numChannels = 1;

    if (*end == '/') {
        s = end + 1;
        unsigned long x = strtoul(s, &end, 10);
        if (end <= s || *end != '\0') {
            return false;
        }

        *numChannels = x;
    }
    return true;
}


bool ASessionDescription::parseNTPRange(
        const char *s, float *npt1, float *npt2) {
    if (s[0] == '-') {
        return false;  
    }

    if (!strncmp("now", s, 3)) {
        return false;  
    }

    char *end;
    *npt1 = strtof(s, &end);

    if (end == s || *end != '-') {
        
        return false;
    }

    s = end + 1;  

    if (!strncmp("now", s, 3)) {
        return false;  
    }

    *npt2 = strtof(s, &end);

    if (end == s) {
        
        return true;
    }

    if (*end != '\0') {
        
        return false;
    }

    return *npt2 > *npt1;
}

}  

