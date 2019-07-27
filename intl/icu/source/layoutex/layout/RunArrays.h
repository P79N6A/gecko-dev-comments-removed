






#ifndef __RUNARRAYS_H

#define __RUNARRAYS_H

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"

#include "unicode/utypes.h"
#include "unicode/locid.h"





 
U_NAMESPACE_BEGIN






#define INITIAL_CAPACITY 16







#define CAPACITY_GROW_LIMIT 128









class U_LAYOUTEX_API RunArray : public UObject
{
public:
    










    inline RunArray(const le_int32 *limits, le_int32 count);

    










    RunArray(le_int32 initialCapacity);

    




    virtual ~RunArray();

    






    inline le_int32 getCount() const;

    







    inline void reset();

    







    inline le_int32 getLimit() const;

    








    inline le_int32 getLimit(le_int32 run) const;

    























    le_int32 add(le_int32 limit);

    




    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    




    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    











    virtual void init(le_int32 capacity);

    











    virtual void grow(le_int32 capacity);

    








    le_bool fClientArrays;

private:
    



    static const char fgClassID;

    le_int32 ensureCapacity();

    inline RunArray();
    inline RunArray(const RunArray & );
    inline RunArray &operator=(const RunArray & ) { return *this; };

    const le_int32 *fLimits;
          le_int32  fCount;
          le_int32  fCapacity;
};

inline RunArray::RunArray()
    : UObject(), fClientArrays(FALSE), fLimits(NULL), fCount(0), fCapacity(0)
{
    
}

inline RunArray::RunArray(const RunArray & )
    : UObject(), fClientArrays(FALSE), fLimits(NULL), fCount(0), fCapacity(0)
{
    
}

inline RunArray::RunArray(const le_int32 *limits, le_int32 count)
    : UObject(), fClientArrays(TRUE), fLimits(limits), fCount(count), fCapacity(count)
{
    
}

inline le_int32 RunArray::getCount() const
{
    return fCount;
}

inline void RunArray::reset()
{
    fCount = 0;
}

inline le_int32 RunArray::getLimit(le_int32 run) const
{
    if (run < 0 || run >= fCount) {
        return -1;
    }

    return fLimits[run];
}

inline le_int32 RunArray::getLimit() const
{
    return getLimit(fCount - 1);
}







class U_LAYOUTEX_API FontRuns : public RunArray
{
public:
    














    inline FontRuns(const LEFontInstance **fonts, const le_int32 *limits, le_int32 count);

    










    FontRuns(le_int32 initialCapacity);

    




    virtual ~FontRuns();

    












    const LEFontInstance *getFont(le_int32 run) const;


    





















    le_int32 add(const LEFontInstance *font, le_int32 limit);

    




    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    




    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline FontRuns();
    inline FontRuns(const FontRuns &other);
    inline FontRuns &operator=(const FontRuns & ) { return *this; };

    



    static const char fgClassID;

    const LEFontInstance **fFonts;
};

inline FontRuns::FontRuns()
    : RunArray(0), fFonts(NULL)
{
    
}

inline FontRuns::FontRuns(const FontRuns & )
    : RunArray(0), fFonts(NULL)
{
    
}

inline FontRuns::FontRuns(const LEFontInstance **fonts, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fFonts(fonts)
{
    
}







class U_LAYOUTEX_API LocaleRuns : public RunArray
{
public:
    














    inline LocaleRuns(const Locale **locales, const le_int32 *limits, le_int32 count);

    










    LocaleRuns(le_int32 initialCapacity);

    




    virtual ~LocaleRuns();

    












    const Locale *getLocale(le_int32 run) const;


    





















    le_int32 add(const Locale *locale, le_int32 limit);

    




    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    




    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

    


    const Locale **fLocales;

private:

    inline LocaleRuns();
    inline LocaleRuns(const LocaleRuns &other);
    inline LocaleRuns &operator=(const LocaleRuns & ) { return *this; };

    



    static const char fgClassID;
};

inline LocaleRuns::LocaleRuns()
    : RunArray(0), fLocales(NULL)
{
    
}

inline LocaleRuns::LocaleRuns(const LocaleRuns & )
    : RunArray(0), fLocales(NULL)
{
    
}

inline LocaleRuns::LocaleRuns(const Locale **locales, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fLocales(locales)
{
    
}






class U_LAYOUTEX_API ValueRuns : public RunArray
{
public:
    













    inline ValueRuns(const le_int32 *values, const le_int32 *limits, le_int32 count);

    










    ValueRuns(le_int32 initialCapacity);

    




    virtual ~ValueRuns();

    












    le_int32 getValue(le_int32 run) const;


    




















    le_int32 add(le_int32 value, le_int32 limit);

    




    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    




    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual void init(le_int32 capacity);
    virtual void grow(le_int32 capacity);

private:

    inline ValueRuns();
    inline ValueRuns(const ValueRuns &other);
    inline ValueRuns &operator=(const ValueRuns & ) { return *this; };

    



    static const char fgClassID;

    const le_int32 *fValues;
};

inline ValueRuns::ValueRuns()
    : RunArray(0), fValues(NULL)
{
    
}

inline ValueRuns::ValueRuns(const ValueRuns & )
    : RunArray(0), fValues(NULL)
{
    
}

inline ValueRuns::ValueRuns(const le_int32 *values, const le_int32 *limits, le_int32 count)
    : RunArray(limits, count), fValues(values)
{
    
}

U_NAMESPACE_END
#endif
