





#ifndef SCINUMBERFORMATTER_H
#define SCINUMBERFORMATTER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#ifndef U_HIDE_DRAFT_API

#include "unicode/unistr.h"






U_NAMESPACE_BEGIN

class FieldPositionIterator;
class DecimalFormatStaticSets;
class DecimalFormatSymbols;
class DecimalFormat;
class Formattable;




















class U_I18N_API ScientificNumberFormatter : public UObject {
public:

    









    static ScientificNumberFormatter *createSuperscriptInstance(
            DecimalFormat *fmtToAdopt, UErrorCode &status);

    








    static ScientificNumberFormatter *createSuperscriptInstance(
            const Locale &locale, UErrorCode &status);


    











    static ScientificNumberFormatter *createMarkupInstance(
            DecimalFormat *fmtToAdopt,
            const UnicodeString &beginMarkup,
            const UnicodeString &endMarkup,
            UErrorCode &status);

    










    static ScientificNumberFormatter *createMarkupInstance(
            const Locale &locale,
            const UnicodeString &beginMarkup,
            const UnicodeString &endMarkup,
            UErrorCode &status);


    



    ScientificNumberFormatter *clone() const {
        return new ScientificNumberFormatter(*this);
    }

    



    virtual ~ScientificNumberFormatter();

    









    UnicodeString &format(
            const Formattable &number,
            UnicodeString &appendTo,
            UErrorCode &status) const;
 private:
    class U_I18N_API Style : public UObject {
    public:
        virtual Style *clone() const = 0;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const = 0;
    private:
        friend class ScientificNumberFormatter;
    };

    class U_I18N_API SuperscriptStyle : public Style {
    public:
        virtual Style *clone() const;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const;
    };

    class U_I18N_API MarkupStyle : public Style {
    public:
        MarkupStyle(
                const UnicodeString &beginMarkup,
                const UnicodeString &endMarkup)
                : Style(),
                  fBeginMarkup(beginMarkup),
                  fEndMarkup(endMarkup) { }
        virtual Style *clone() const;
    protected:
        virtual UnicodeString &format(
                const UnicodeString &original,
                FieldPositionIterator &fpi,
                const UnicodeString &preExponent,
                const DecimalFormatStaticSets &decimalFormatSets,
                UnicodeString &appendTo,
                UErrorCode &status) const;
    private:
        UnicodeString fBeginMarkup;
        UnicodeString fEndMarkup;
    };

    ScientificNumberFormatter(
            DecimalFormat *fmtToAdopt,
            Style *styleToAdopt,
            UErrorCode &status);

    ScientificNumberFormatter(const ScientificNumberFormatter &other);
    ScientificNumberFormatter &operator=(const ScientificNumberFormatter &);

    static void getPreExponent(
            const DecimalFormatSymbols &dfs, UnicodeString &preExponent);

    static ScientificNumberFormatter *createInstance(
            DecimalFormat *fmtToAdopt,
            Style *styleToAdopt,
            UErrorCode &status);

    UnicodeString fPreExponent;
    DecimalFormat *fDecimalFormat;
    Style *fStyle;
    const DecimalFormatStaticSets *fStaticSets;

};

U_NAMESPACE_END

#endif 

#endif 
#endif 
