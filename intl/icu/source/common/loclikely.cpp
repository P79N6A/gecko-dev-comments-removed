


















#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "cmemory.h"
#include "cstring.h"
#include "ulocimp.h"
#include "ustr_imp.h"









static const char*  U_CALLCONV
findLikelySubtags(const char* localeID,
                  char* buffer,
                  int32_t bufferLength,
                  UErrorCode* err) {
    const char* result = NULL;

    if (!U_FAILURE(*err)) {
        int32_t resLen = 0;
        const UChar* s = NULL;
        UErrorCode tmpErr = U_ZERO_ERROR;
        UResourceBundle* subtags = ures_openDirect(NULL, "likelySubtags", &tmpErr);
        if (U_SUCCESS(tmpErr)) {
            s = ures_getStringByKey(subtags, localeID, &resLen, &tmpErr);

            if (U_FAILURE(tmpErr)) {
                



                if (tmpErr != U_MISSING_RESOURCE_ERROR) {
                    *err = tmpErr;
                }
            }
            else if (resLen >= bufferLength) {
                
                *err = U_INTERNAL_PROGRAM_ERROR;
            }
            else {
                u_UCharsToChars(s, buffer, resLen + 1);
                result = buffer;
            }

            ures_close(subtags);
        } else {
            *err = tmpErr;
        }
    }

    return result;
}











static void U_CALLCONV
appendTag(
    const char* tag,
    int32_t tagLength,
    char* buffer,
    int32_t* bufferLength) {

    if (*bufferLength > 0) {
        buffer[*bufferLength] = '_';
        ++(*bufferLength);
    }

    uprv_memmove(
        &buffer[*bufferLength],
        tag,
        tagLength);

    *bufferLength += tagLength;
}




static const char* const unknownLanguage = "und";
static const char* const unknownScript = "Zzzz";
static const char* const unknownRegion = "ZZ";



































static int32_t U_CALLCONV
createTagStringWithAlternates(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    const char* alternateTags,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err) {

    if (U_FAILURE(*err)) {
        goto error;
    }
    else if (tag == NULL ||
             tagCapacity <= 0 ||
             langLength >= ULOC_LANG_CAPACITY ||
             scriptLength >= ULOC_SCRIPT_CAPACITY ||
             regionLength >= ULOC_COUNTRY_CAPACITY) {
        goto error;
    }
    else {
        





        char tagBuffer[ULOC_FULLNAME_CAPACITY];
        int32_t tagLength = 0;
        int32_t capacityRemaining = tagCapacity;
        UBool regionAppended = FALSE;

        if (langLength > 0) {
            appendTag(
                lang,
                langLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags == NULL) {
            



            appendTag(
                unknownLanguage,
                (int32_t)uprv_strlen(unknownLanguage),
                tagBuffer,
                &tagLength);
        }
        else {
            


            char alternateLang[ULOC_LANG_CAPACITY];
            int32_t alternateLangLength = sizeof(alternateLang);

            alternateLangLength =
                uloc_getLanguage(
                    alternateTags,
                    alternateLang,
                    alternateLangLength,
                    err);
            if(U_FAILURE(*err) ||
                alternateLangLength >= ULOC_LANG_CAPACITY) {
                goto error;
            }
            else if (alternateLangLength == 0) {
                



                appendTag(
                    unknownLanguage,
                    (int32_t)uprv_strlen(unknownLanguage),
                    tagBuffer,
                    &tagLength);
            }
            else {
                appendTag(
                    alternateLang,
                    alternateLangLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        if (scriptLength > 0) {
            appendTag(
                script,
                scriptLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags != NULL) {
            


            char alternateScript[ULOC_SCRIPT_CAPACITY];

            const int32_t alternateScriptLength =
                uloc_getScript(
                    alternateTags,
                    alternateScript,
                    sizeof(alternateScript),
                    err);

            if (U_FAILURE(*err) ||
                alternateScriptLength >= ULOC_SCRIPT_CAPACITY) {
                goto error;
            }
            else if (alternateScriptLength > 0) {
                appendTag(
                    alternateScript,
                    alternateScriptLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        if (regionLength > 0) {
            appendTag(
                region,
                regionLength,
                tagBuffer,
                &tagLength);

            regionAppended = TRUE;
        }
        else if (alternateTags != NULL) {
            


            char alternateRegion[ULOC_COUNTRY_CAPACITY];

            const int32_t alternateRegionLength =
                uloc_getCountry(
                    alternateTags,
                    alternateRegion,
                    sizeof(alternateRegion),
                    err);
            if (U_FAILURE(*err) ||
                alternateRegionLength >= ULOC_COUNTRY_CAPACITY) {
                goto error;
            }
            else if (alternateRegionLength > 0) {
                appendTag(
                    alternateRegion,
                    alternateRegionLength,
                    tagBuffer,
                    &tagLength);

                regionAppended = TRUE;
            }
        }

        {
            const int32_t toCopy =
                tagLength >= tagCapacity ? tagCapacity : tagLength;

            



            uprv_memcpy(
                tag,
                tagBuffer,
                toCopy);

            capacityRemaining -= toCopy;
        }

        if (trailingLength > 0) {
            if (*trailing != '@' && capacityRemaining > 0) {
                tag[tagLength++] = '_';
                --capacityRemaining;
                if (capacityRemaining > 0 && !regionAppended) {
                    
                    tag[tagLength++] = '_';
                    --capacityRemaining;
                }
            }

            if (capacityRemaining > 0) {
                



                const int32_t toCopy =
                    trailingLength >= capacityRemaining ? capacityRemaining : trailingLength;

                uprv_memmove(
                    &tag[tagLength],
                    trailing,
                    toCopy);
            }
        }

        tagLength += trailingLength;

        return u_terminateChars(
                    tag,
                    tagCapacity,
                    tagLength,
                    err);
    }

error:

    




    if (*err ==  U_BUFFER_OVERFLOW_ERROR ||
        U_SUCCESS(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}



























static int32_t U_CALLCONV
createTagString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    return createTagStringWithAlternates(
                lang,
                langLength,
                script,
                scriptLength,
                region,
                regionLength,
                trailing,
                trailingLength,
                NULL,
                tag,
                tagCapacity,
                err);
}





























static int32_t U_CALLCONV
parseTagString(
    const char* localeID,
    char* lang,
    int32_t* langLength,
    char* script,
    int32_t* scriptLength,
    char* region,
    int32_t* regionLength,
    UErrorCode* err)
{
    const char* position = localeID;
    int32_t subtagLength = 0;

    if(U_FAILURE(*err) ||
       localeID == NULL ||
       lang == NULL ||
       langLength == NULL ||
       script == NULL ||
       scriptLength == NULL ||
       region == NULL ||
       regionLength == NULL) {
        goto error;
    }

    subtagLength = ulocimp_getLanguage(position, lang, *langLength, &position);
    u_terminateChars(lang, *langLength, subtagLength, err);

    




    if(U_FAILURE(*err)) {
        goto error;
    }

    *langLength = subtagLength;

    



    if (*langLength == 0) {
        uprv_strcpy(
            lang,
            unknownLanguage);
        *langLength = (int32_t)uprv_strlen(lang);
    }
    else if (_isIDSeparator(*position)) {
        ++position;
    }

    subtagLength = ulocimp_getScript(position, script, *scriptLength, &position);
    u_terminateChars(script, *scriptLength, subtagLength, err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    *scriptLength = subtagLength;

    if (*scriptLength > 0) {
        if (uprv_strnicmp(script, unknownScript, *scriptLength) == 0) {
            


            *scriptLength = 0;
        }

        


        if (_isIDSeparator(*position)) {
            ++position;
        }    
    }

    subtagLength = ulocimp_getCountry(position, region, *regionLength, &position);
    u_terminateChars(region, *regionLength, subtagLength, err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    *regionLength = subtagLength;

    if (*regionLength > 0) {
        if (uprv_strnicmp(region, unknownRegion, *regionLength) == 0) {
            


            *regionLength = 0;
        }
    } else if (*position != 0 && *position != '@') {
        
        --position;
    }

exit:

    return (int32_t)(position - localeID);

error:

    



    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    goto exit;
}

static int32_t U_CALLCONV
createLikelySubtagsString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* variants,
    int32_t variantsLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    





    char tagBuffer[ULOC_FULLNAME_CAPACITY];
    char likelySubtagsBuffer[ULOC_FULLNAME_CAPACITY];

    if(U_FAILURE(*err)) {
        goto error;
    }

    


    if (scriptLength > 0 && regionLength > 0) {

        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            


            return createTagStringWithAlternates(
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    


    if (scriptLength > 0) {

        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            


            return createTagStringWithAlternates(
                        NULL,
                        0,
                        NULL,
                        0,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    


    if (regionLength > 0) {

        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            


            return createTagStringWithAlternates(
                        NULL,
                        0,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    


    {
        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            


            return createTagStringWithAlternates(
                        NULL,
                        0,
                        script,
                        scriptLength,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    return u_terminateChars(
                tag,
                tagCapacity,
                0,
                err);

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

#define CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength) \
    {   int32_t count = 0; \
        int32_t i; \
        for (i = 0; i < trailingLength; i++) { \
            if (trailing[i] == '-' || trailing[i] == '_') { \
                count = 0; \
                if (count > 8) { \
                    goto error; \
                } \
            } else if (trailing[i] == '@') { \
                break; \
            } else if (count > 8) { \
                goto error; \
            } else { \
                count++; \
            } \
        } \
    }

static int32_t
_uloc_addLikelySubtags(const char*    localeID,
         char* maximizedLocaleID,
         int32_t maximizedLocaleIDCapacity,
         UErrorCode* err)
{
    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* trailing = "";
    int32_t trailingLength = 0;
    int32_t trailingIndex = 0;
    int32_t resultLength = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             maximizedLocaleID == NULL ||
             maximizedLocaleIDCapacity <= 0) {
        goto error;
    }

    trailingIndex = parseTagString(
        localeID,
        lang,
        &langLength,
        script,
        &scriptLength,
        region,
        &regionLength,
        err);
    if(U_FAILURE(*err)) {
        
        if (*err == U_BUFFER_OVERFLOW_ERROR) {
            *err = U_ILLEGAL_ARGUMENT_ERROR;
        }

        goto error;
    }

    
    while (_isIDSeparator(localeID[trailingIndex])) {
        trailingIndex++;
    }
    trailing = &localeID[trailingIndex];
    trailingLength = (int32_t)uprv_strlen(trailing);

    CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength);

    resultLength =
        createLikelySubtagsString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            trailing,
            trailingLength,
            maximizedLocaleID,
            maximizedLocaleIDCapacity,
            err);

    if (resultLength == 0) {
        const int32_t localIDLength = (int32_t)uprv_strlen(localeID);

        


        uprv_memcpy(
            maximizedLocaleID,
            localeID,
            localIDLength <= maximizedLocaleIDCapacity ? 
                localIDLength : maximizedLocaleIDCapacity);

        resultLength =
            u_terminateChars(
                maximizedLocaleID,
                maximizedLocaleIDCapacity,
                localIDLength,
                err);
    }

    return resultLength;

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

static int32_t
_uloc_minimizeSubtags(const char*    localeID,
         char* minimizedLocaleID,
         int32_t minimizedLocaleIDCapacity,
         UErrorCode* err)
{
    





    char maximizedTagBuffer[ULOC_FULLNAME_CAPACITY];
    int32_t maximizedTagBufferLength = sizeof(maximizedTagBuffer);

    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* trailing = "";
    int32_t trailingLength = 0;
    int32_t trailingIndex = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             minimizedLocaleID == NULL ||
             minimizedLocaleIDCapacity <= 0) {
        goto error;
    }

    trailingIndex =
        parseTagString(
            localeID,
            lang,
            &langLength,
            script,
            &scriptLength,
            region,
            &regionLength,
            err);
    if(U_FAILURE(*err)) {

        
        if (*err == U_BUFFER_OVERFLOW_ERROR) {
            *err = U_ILLEGAL_ARGUMENT_ERROR;
        }

        goto error;
    }

    
    while (_isIDSeparator(localeID[trailingIndex])) {
        trailingIndex++;
    }
    trailing = &localeID[trailingIndex];
    trailingLength = (int32_t)uprv_strlen(trailing);

    CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength);

    createTagString(
        lang,
        langLength,
        script,
        scriptLength,
        region,
        regionLength,
        NULL,
        0,
        maximizedTagBuffer,
        maximizedTagBufferLength,
        err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    



    maximizedTagBufferLength =
        uloc_addLikelySubtags(
            maximizedTagBuffer,
            maximizedTagBuffer,
            maximizedTagBufferLength,
            err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    


    {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        NULL,
                        0,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    


    if (regionLength > 0) {

        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                region,
                regionLength,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        region,
                        regionLength,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    




    if (scriptLength > 0 && regionLength > 0) {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                script,
                scriptLength,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    {
        


        const int32_t localeIDLength = (int32_t)uprv_strlen(localeID);

        uprv_memcpy(
            minimizedLocaleID,
            localeID,
            localeIDLength <= minimizedLocaleIDCapacity ? 
                localeIDLength : minimizedLocaleIDCapacity);

        return u_terminateChars(
                    minimizedLocaleID,
                    minimizedLocaleIDCapacity,
                    localeIDLength,
                    err);
    }

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;


}

static UBool
do_canonicalize(const char*    localeID,
         char* buffer,
         int32_t bufferCapacity,
         UErrorCode* err)
{
    uloc_canonicalize(
        localeID,
        buffer,
        bufferCapacity,
        err);

    if (*err == U_STRING_NOT_TERMINATED_WARNING ||
        *err == U_BUFFER_OVERFLOW_ERROR) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;

        return FALSE;
    }
    else if (U_FAILURE(*err)) {

        return FALSE;
    }
    else {
        return TRUE;
    }
}

U_CAPI int32_t U_EXPORT2
uloc_addLikelySubtags(const char*    localeID,
         char* maximizedLocaleID,
         int32_t maximizedLocaleIDCapacity,
         UErrorCode* err)
{
    char localeBuffer[ULOC_FULLNAME_CAPACITY];

    if (!do_canonicalize(
        localeID,
        localeBuffer,
        sizeof(localeBuffer),
        err)) {
        return -1;
    }
    else {
        return _uloc_addLikelySubtags(
                    localeBuffer,
                    maximizedLocaleID,
                    maximizedLocaleIDCapacity,
                    err);
    }    
}

U_CAPI int32_t U_EXPORT2
uloc_minimizeSubtags(const char*    localeID,
         char* minimizedLocaleID,
         int32_t minimizedLocaleIDCapacity,
         UErrorCode* err)
{
    char localeBuffer[ULOC_FULLNAME_CAPACITY];

    if (!do_canonicalize(
        localeID,
        localeBuffer,
        sizeof(localeBuffer),
        err)) {
        return -1;
    }
    else {
        return _uloc_minimizeSubtags(
                    localeBuffer,
                    minimizedLocaleID,
                    minimizedLocaleIDCapacity,
                    err);
    }    
}
