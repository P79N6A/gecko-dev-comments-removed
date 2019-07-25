

























#pragma once
#include <cstring>
#include <cassert>

namespace graphite2 {

struct IsoLangEntry
{
    unsigned short mnLang;
    const char maLangStr[4];
    const char maCountry[3];
};



const IsoLangEntry LANG_ENTRIES[] = {
    { 0x0401, "ar","SA" }, 
    { 0x0402, "bg","BG" }, 
    { 0x0403, "ca","ES" }, 
    { 0x0404, "zh","TW" }, 
    { 0x0405, "cs","CZ" }, 
    { 0x0406, "da","DK" }, 
    { 0x0407, "de","DE" }, 
    { 0x0408, "el","GR" }, 
    { 0x0409, "en","US" }, 
    { 0x040A, "es","ES" }, 
    { 0x040B, "fi","FI" }, 
    { 0x040C, "fr","FR" }, 
    { 0x040D, "he","IL" }, 
    { 0x040E, "hu","HU" }, 
    { 0x040F, "is","IS" }, 
    { 0x0410, "it","IT" }, 
    { 0x0411, "jp","JP" }, 
    { 0x0412, "ko","KR" }, 
    { 0x0413, "nl","NL" }, 
    { 0x0414, "no","NO" }, 
    { 0x0415, "pl","PL" }, 
    { 0x0416, "pt","BR" }, 
    { 0x0417, "rm","CH" }, 
    { 0x0418, "ro","RO" }, 
    { 0x0419, "ru","RU" }, 
    { 0x041A, "hr","HR" }, 
    { 0x041B, "sk","SK" }, 
    { 0x041C, "sq","AL" }, 
    { 0x041D, "sv","SE" }, 
    { 0x041E, "th","TH" }, 
    { 0x041F, "tr","TR" }, 
    { 0x0420, "ur","PK" }, 
    { 0x0421, "id","ID" }, 
    { 0x0422, "uk","UA" }, 
    { 0x0423, "be","BY" }, 
    { 0x0424, "sl","SI" }, 
    { 0x0425, "et","EE" }, 
    { 0x0426, "lv","LV" }, 
    { 0x0427, "lt","LT" }, 
    { 0x0428, "tg","TJ" }, 
    { 0x042A, "vi","VN" }, 
    { 0x042B, "hy","AM" }, 
    { 0x042C, "az","AZ" }, 
    { 0x042D, "eu","" }, 
    { 0x042E, "hsb","DE" }, 
    { 0x042F, "mk","MK" }, 
    { 0x0432, "tn","ZA" }, 
    { 0x0434, "xh","ZA" }, 
    { 0x0435, "zu","ZA" }, 
    { 0x0436, "af","ZA" }, 
    { 0x0437, "ka","GE" }, 
    { 0x0438, "fo","FO" }, 
    { 0x0439, "hi","IN" }, 
    { 0x043A, "mt","MT" }, 
    { 0x043B, "se","NO" }, 
    { 0x043E, "ms","MY" }, 
    { 0x043F, "kk","KZ" }, 
    { 0x0440, "ky","KG" }, 
    { 0x0441, "sw","KE" }, 
    { 0x0442, "tk","TM" }, 
    { 0x0443, "uz","UZ" }, 
    { 0x0444, "tt","RU" }, 
    { 0x0445, "bn","IN" }, 
    { 0x0446, "pa","IN" }, 
    { 0x0447, "gu","IN" }, 
    { 0x0448, "or","IN" }, 
    { 0x0448, "wo","SN" }, 
    { 0x0449, "ta","IN" }, 
    { 0x044A, "te","IN" }, 
    { 0x044B, "kn","IN" }, 
    { 0x044C, "ml","IN" }, 
    { 0x044D, "as","IN" }, 
    { 0x044E, "mr","IN" }, 
    { 0x044F, "sa","IN" }, 
    { 0x0450, "mn","MN" }, 
    { 0x0451, "bo","CN" }, 
    { 0x0452, "cy","GB" }, 
    { 0x0453, "km","KH" }, 
    { 0x0454, "lo","LA" }, 
    { 0x0455, "my","MM" }, 
    { 0x0456, "gl","ES" }, 
    { 0x0457, "kok","IN" }, 
    { 0x045A, "syr","TR" }, 
    { 0x045B, "si","LK" }, 
    { 0x045D, "iu","CA" }, 
    { 0x045E, "am","ET" }, 
    { 0x0461, "ne","NP" }, 
    { 0x0462, "fy","NL" }, 
    { 0x0463, "ps","AF" }, 
    { 0x0464, "fil","PH" }, 
    { 0x0465, "dv","MV" }, 
    { 0x0468, "ha","NG" }, 
    { 0x046A, "yo","NG" }, 
    { 0x046B, "qu","BO" }, 
    { 0x046C, "st","ZA" }, 
    { 0x046D, "ba","RU" }, 
    { 0x046E, "lb","LU" }, 
    { 0x046F, "kl","GL" }, 
    { 0x0470, "ig","NG" }, 
    { 0x0478, "ii","CN" }, 
    { 0x047A, "arn","CL" }, 
    { 0x047C, "moh","CA" }, 
    { 0x047E, "br","FR" }, 
    { 0x0480, "ug","CN" }, 
    { 0x0481, "mi","NZ" }, 
    { 0x0482, "oc","FR" }, 
    { 0x0483, "co","FR" }, 
    { 0x0484, "gsw","FR" }, 
    { 0x0485, "sah","RU" }, 
    { 0x0486, "qut","GT" }, 
    { 0x0487, "rw","RW" }, 
    { 0x048C, "gbz","AF" }, 
    { 0x0801, "ar","IQ" }, 
    { 0x0804, "zn","CH" }, 
    { 0x0807, "de","CH" }, 
    { 0x0809, "en","GB" }, 
    { 0x080A, "es","MX" }, 
    { 0x080C, "fr","BE" }, 
    { 0x0810, "it","CH" }, 
    { 0x0813, "nl","BE" }, 
    { 0x0814, "nn","NO" }, 
    { 0x0816, "pt","PT" }, 
    { 0x081A, "sh","RS" }, 
    { 0x081D, "sv","FI" }, 
    { 0x082C, "az","AZ" }, 
    { 0x082E, "dsb","DE" }, 
    { 0x083B, "se","SE" }, 
    { 0x083C, "ga","IE" }, 
    { 0x083E, "ms","BN" }, 
    { 0x0843, "uz","UZ" }, 
    { 0x0845, "bn","BD" }, 
    { 0x0850, "mn","MN" }, 
    { 0x085D, "iu","CA" }, 
    { 0x085F, "ber","DZ" }, 
    { 0x086B, "es","EC" }, 
    { 0x0C01, "ar","EG" }, 
    { 0x0C04, "zh","HK" }, 
    { 0x0C07, "de","AT" }, 
    { 0x0C09, "en","AU" }, 
    { 0x0C0A, "es","ES" }, 
    { 0x0C0C, "fr","CA" }, 
    { 0x0C1A, "sr","CS" }, 
    { 0x0C3B, "se","FI" }, 
    { 0x0C6B, "qu","PE" }, 
    { 0x1001, "ar","LY" }, 
    { 0x1004, "zh","SG" }, 
    { 0x1007, "de","LU" }, 
    { 0x1009, "en","CA" }, 
    { 0x100A, "es","GT" }, 
    { 0x100C, "fr","CH" }, 
    { 0x101A, "hr","BA" }, 
    { 0x103B, "smj","NO" }, 
    { 0x1401, "ar","DZ" }, 
    { 0x1404, "zh","MO" }, 
    { 0x1407, "de","LI" }, 
    { 0x1409, "en","NZ" }, 
    { 0x140A, "es","CR" }, 
    { 0x140C, "fr","LU" }, 
    { 0x141A, "bs","BA" }, 
    { 0x143B, "smj","SE" }, 
    { 0x1801, "ar","MA" }, 
    { 0x1809, "en","IE" }, 
    { 0x180A, "es","PA" }, 
    { 0x180C, "fr","MC" }, 
    { 0x181A, "sh","BA" }, 
    { 0x183B, "sma","NO" }, 
    { 0x1C01, "ar","TN" }, 
    { 0x1C09, "en","ZA" }, 
    { 0x1C0A, "es","DO" }, 
    { 0x1C1A, "sr","BA" }, 
    { 0x1C3B, "sma","SE" }, 
    { 0x2001, "ar","OM" }, 
    { 0x2009, "en","JM" }, 
    { 0x200A, "es","VE" }, 
    { 0x201A, "bs","BA" }, 
    { 0x203B, "sms","FI" }, 
    { 0x2401, "ar","YE" }, 
    { 0x2409, "en","BS" }, 
    { 0x240A, "es","CO" }, 
    { 0x243B, "smn","FI" }, 
    { 0x2801, "ar","SY" }, 
    { 0x2809, "en","BZ" }, 
    { 0x280A, "es","PE" }, 
    { 0x2C01, "ar","JO" }, 
    { 0x2C09, "en","TT" }, 
    { 0x2C0A, "es","AR" }, 
    { 0x3001, "ar","LB" }, 
    { 0x3009, "en","ZW" }, 
    { 0x300A, "es","EC" }, 
    { 0x3401, "ar","KW" }, 
    { 0x3409, "en","PH" }, 
    { 0x340A, "es","CL" }, 
    { 0x3801, "ar","AE" }, 
    { 0x380A, "es","UY" }, 
    { 0x3C01, "ar","BH" }, 
    { 0x3C0A, "es","PY" }, 
    { 0x4001, "ar","QA" }, 
    { 0x4009, "en","IN" }, 
    { 0x400A, "es","BO" }, 
    { 0x4409, "en","MY" }, 
    { 0x440A, "es","SV" }, 
    { 0x4809, "en","SG" }, 
    { 0x480A, "es","HN" }, 
    { 0x4C0A, "es","NI" }, 
    { 0x500A, "es","PR" }, 
    { 0x540A, "es","US" } 
};

class Locale2Lang
{
public:
    Locale2Lang() : mSeedPosition(128)
    {
        memset((void*)mLangLookup, 0, sizeof(mLangLookup));
        
        static const int maxIndex = sizeof(LANG_ENTRIES)/sizeof(IsoLangEntry);
        for (int i = 0; i < maxIndex; i++)
        {
            size_t a = LANG_ENTRIES[i].maLangStr[0] - 'a';
            size_t b = LANG_ENTRIES[i].maLangStr[1] - 'a';
            if (mLangLookup[a][b])
            {
                const IsoLangEntry ** old = mLangLookup[a][b];
                int len = 1;
                while (old[len]) len++;
                len += 2;
                mLangLookup[a][b] = gralloc<const IsoLangEntry *>(len);
                mLangLookup[a][b][--len] = NULL;
                mLangLookup[a][b][--len] = &LANG_ENTRIES[i];
                while (--len >= 0)
                {
                    assert(len >= 0);
                    mLangLookup[a][b][len] = old[len];
                }
                free(old);
            }
            else
            {
                mLangLookup[a][b] = gralloc<const IsoLangEntry *>(2);
                mLangLookup[a][b][1] = NULL;
                mLangLookup[a][b][0] = &LANG_ENTRIES[i];
            }
        }
        while (2 * mSeedPosition < maxIndex)
            mSeedPosition *= 2;
    };
    ~Locale2Lang()
    {
        for (int i = 0; i != 26; ++i)
        	for (int j = 0; j != 26; ++j)
        		free(mLangLookup[i][j]);
    }
    unsigned short getMsId(const char * locale) const
    {
        size_t length = strlen(locale);
        size_t langLength = length;
        const char * language = locale;
        const char * script = NULL;
        const char * region = NULL;
        size_t regionLength = 0;
        const char * dash = strchr(locale, '-');
        if (dash && (dash != locale))
        {
            langLength = (dash - locale);
            size_t nextPartLength = length - langLength - 1;
            if (nextPartLength >= 2)
            {
                script = ++dash;
                dash = strchr(dash, '-');
                if (dash)
                {
                    nextPartLength = (dash - script);
                    region = ++dash;
                }
                if (nextPartLength == 2 &&
                    (locale[langLength+1] > 0x40) && (locale[langLength+1] < 0x5B) &&
                    (locale[langLength+2] > 0x40) && (locale[langLength+2] < 0x5B))
                {
                    region = script;
                    regionLength = nextPartLength;
                    script = NULL;
                }
                else if (nextPartLength == 4)
                {
                    if (dash)
                    {
                        dash = strchr(dash, '-');
                        if (dash)
                        {
                            nextPartLength = (dash - region);
                        }
                        else
                        {
                            nextPartLength = langLength - (region - locale);
                        }
                        regionLength = nextPartLength;
                    }
                }
            }
        }
        size_t a = 'e' - 'a';
        size_t b = 'n' - 'a';
        unsigned short langId = 0;
        int i = 0;
        switch (langLength)
        {
            case 2:
            {
                a = language[0] - 'a';
                b = language[1] - 'a';
                if ((a < 26) && (b < 26) && mLangLookup[a][b])
                {
                    while (mLangLookup[a][b][i])
                    {
                        if (mLangLookup[a][b][i]->maLangStr[2] != '\0')
                        {
                            ++i;
                            continue;
                        }
                        if (region && (strncmp(mLangLookup[a][b][i]->maCountry, region, regionLength) == 0))
                        {
                            langId = mLangLookup[a][b][i]->mnLang;
                            break;
                        }
                        else if (langId == 0)
                        {
                            
                            langId = mLangLookup[a][b][i]->mnLang;
                        }
                        ++i;
                    }
                }
            }
            break;
            case 3:
            {
                a = language[0] - 'a';
                b = language[1] - 'a';
                if (mLangLookup[a][b])
                {
                    while (mLangLookup[a][b][i])
                    {
                        if (mLangLookup[a][b][i]->maLangStr[2] != language[2])
                        {
                            ++i;
                            continue;
                        }
                        if (strcmp(mLangLookup[a][b][i]->maCountry, region) == 0)
                        {
                            langId = mLangLookup[a][b][i]->mnLang;
                            break;
                        }
                        else if (langId == 0)
                        {
                            
                            langId = mLangLookup[a][b][i]->mnLang;
                        }
                        ++i;
                    }
                }
            }
            break;
            default:
                break;
        }
        if (langId == 0) langId = 0x409;
        return langId;
    }
    const IsoLangEntry * findEntryById(unsigned short langId) const
    {
        static const int maxIndex = sizeof(LANG_ENTRIES)/sizeof(IsoLangEntry);
        int window = mSeedPosition;
        int guess = mSeedPosition - 1;
        while (LANG_ENTRIES[guess].mnLang != langId)
        {
            window /= 2;
            if (window == 0) return NULL;
            guess += (LANG_ENTRIES[guess].mnLang > langId)? -window : window;
            while (guess >= maxIndex)
            {
                window /= 2;
                guess -= window;
                assert(window);
            }
        }
        return &LANG_ENTRIES[guess];
    }
    CLASS_NEW_DELETE

private:
    const IsoLangEntry ** mLangLookup[26][26];
    int mSeedPosition;
};

} 
