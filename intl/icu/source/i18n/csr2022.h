






#ifndef __CSR2022_H
#define __CSR2022_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN

class CharsetMatch;











class CharsetRecog_2022 : public CharsetRecognizer
{

public:    
    virtual ~CharsetRecog_2022() = 0;

protected:

    











    int32_t match_2022(const uint8_t *text,
                       int32_t textLen,
                       const uint8_t escapeSequences[][5],
                       int32_t escapeSequences_length) const;

};

class CharsetRecog_2022JP :public CharsetRecog_2022
{
public:
    virtual ~CharsetRecog_2022JP();

    const char *getName() const;

    UBool match(InputText *textIn, CharsetMatch *results) const;
};

class CharsetRecog_2022KR :public CharsetRecog_2022 {
public:
    virtual ~CharsetRecog_2022KR();

    const char *getName() const;

    UBool match(InputText *textIn, CharsetMatch *results) const;

};

class CharsetRecog_2022CN :public CharsetRecog_2022
{
public:
    virtual ~CharsetRecog_2022CN();

    const char* getName() const;

    UBool match(InputText *textIn, CharsetMatch *results) const;
};

U_NAMESPACE_END

#endif
#endif 
