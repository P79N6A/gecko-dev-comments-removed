






#ifndef __CSRMBCS_H
#define __CSRMBCS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN












class IteratedChar : public UMemory
{
public:
    uint32_t charValue;             
    int32_t  index;
    int32_t  nextIndex;
    UBool    error;
    UBool    done;

public:
    IteratedChar();
    
    int32_t nextByte(InputText* det);
};


class CharsetRecog_mbcs : public CharsetRecognizer {

protected:
    











    int32_t match_mbcs(InputText* det, const uint16_t commonChars[], int32_t commonCharsLen) const;

public:

    virtual ~CharsetRecog_mbcs();

    




    const char *getName() const = 0;
    const char *getLanguage() const = 0;
    UBool match(InputText* input, CharsetMatch *results) const = 0;

    











    virtual UBool nextChar(IteratedChar *it, InputText *textIn) const = 0;

};






class CharsetRecog_sjis : public CharsetRecog_mbcs {
public:
    virtual ~CharsetRecog_sjis();

    UBool nextChar(IteratedChar *it, InputText *det) const;

    UBool match(InputText* input, CharsetMatch *results) const;

    const char *getName() const;
    const char *getLanguage() const;

};








class CharsetRecog_euc : public CharsetRecog_mbcs
{
public:
    virtual ~CharsetRecog_euc();

    const char *getName() const = 0;
    const char *getLanguage() const = 0;

    UBool match(InputText* input, CharsetMatch *results) const = 0;
    





    UBool nextChar(IteratedChar *it, InputText *det) const;
};





class CharsetRecog_euc_jp : public CharsetRecog_euc
{
public:
    virtual ~CharsetRecog_euc_jp();

    const char *getName() const;
    const char *getLanguage() const;

    UBool match(InputText* input, CharsetMatch *results) const;
};





class CharsetRecog_euc_kr : public CharsetRecog_euc
{
public:
    virtual ~CharsetRecog_euc_kr();

    const char *getName() const;
    const char *getLanguage() const;

    UBool match(InputText* input, CharsetMatch *results) const;
};






class CharsetRecog_big5 : public CharsetRecog_mbcs
{
public:
    virtual ~CharsetRecog_big5();

    UBool nextChar(IteratedChar* it, InputText* det) const;

    const char *getName() const;
    const char *getLanguage() const;

    UBool match(InputText* input, CharsetMatch *results) const;
};







class CharsetRecog_gb_18030 : public CharsetRecog_mbcs
{
public:
    virtual ~CharsetRecog_gb_18030();

    UBool nextChar(IteratedChar* it, InputText* det) const;

    const char *getName() const;
    const char *getLanguage() const;

    UBool match(InputText* input, CharsetMatch *results) const;
};

U_NAMESPACE_END

#endif
#endif 
