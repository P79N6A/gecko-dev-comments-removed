






#ifndef __CSRUCODE_H
#define __CSRUCODE_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN







class CharsetRecog_Unicode : public CharsetRecognizer 
{

public:

    virtual ~CharsetRecog_Unicode();
    


    const char* getName() const = 0;

    


    UBool match(InputText* textIn, CharsetMatch *results) const = 0;
};


class CharsetRecog_UTF_16_BE : public CharsetRecog_Unicode
{
public:

    virtual ~CharsetRecog_UTF_16_BE();

    const char *getName() const;

    UBool match(InputText* textIn, CharsetMatch *results) const;
};

class CharsetRecog_UTF_16_LE : public CharsetRecog_Unicode
{
public:

    virtual ~CharsetRecog_UTF_16_LE();

    const char *getName() const;

    UBool match(InputText* textIn, CharsetMatch *results) const;
};

class CharsetRecog_UTF_32 : public CharsetRecog_Unicode
{
protected:
    virtual int32_t getChar(const uint8_t *input, int32_t index) const = 0;
public:

    virtual ~CharsetRecog_UTF_32();

    const char* getName() const = 0;

    UBool match(InputText* textIn, CharsetMatch *results) const;
};


class CharsetRecog_UTF_32_BE : public CharsetRecog_UTF_32
{
protected:
    int32_t getChar(const uint8_t *input, int32_t index) const;

public:

    virtual ~CharsetRecog_UTF_32_BE();

    const char *getName() const;
};


class CharsetRecog_UTF_32_LE : public CharsetRecog_UTF_32
{
protected:
    int32_t getChar(const uint8_t *input, int32_t index) const;

public:
    virtual ~CharsetRecog_UTF_32_LE();

    const char* getName() const;
};

U_NAMESPACE_END

#endif
#endif 
