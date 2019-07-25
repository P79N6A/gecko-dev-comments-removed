

























#pragma once

#include <graphite2/Segment.h>
#include "TtfTypes.h"
#include "locale2lcid.h"

namespace graphite2 {

class NameTable
{
public:
    NameTable(const void * data, size_t length, uint16 platfromId=3, uint16 encodingID = 1);
    ~NameTable() { free(const_cast<TtfUtil::Sfnt::FontNames *>(m_table)); }
    enum eNameFallback {
        eNoFallback = 0,
        eEnUSFallbackOnly = 1,
        eEnOrAnyFallback = 2
    };
    uint16 setPlatformEncoding(uint16 platfromId=3, uint16 encodingID = 1);
    void * getName(uint16 & languageId, uint16 nameId, gr_encform enc, uint32 & length);
    uint16 getLanguageId(const char * bcp47Locale);

    CLASS_NEW_DELETE
private:
    uint16 m_platformId;
    uint16 m_encodingId;
    uint16 m_languageCount;
    uint16 m_platformOffset; 
    uint16 m_platformLastRecord;
    uint16 m_nameDataLength;
    const TtfUtil::Sfnt::FontNames * m_table;
    const uint8 * m_nameData;
    Locale2Lang m_locale2Lang;
};

} 
