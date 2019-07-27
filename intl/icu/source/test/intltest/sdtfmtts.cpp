






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "sdtfmtts.h"

#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"





void IntlTestSimpleDateFormatAPI::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    if (exec) logln("TestSuite SimpleDateFormatAPI");
    switch (index) {
        case 0: name = "SimpleDateFormat API test"; 
                if (exec) {
                    logln("SimpleDateFormat API test---"); logln("");
                    UErrorCode status = U_ZERO_ERROR;
                    Locale saveLocale;
                    Locale::setDefault(Locale::getEnglish(), status);
                    if(U_FAILURE(status)) {
                        errln("ERROR: Could not set default locale, test may not give correct results");
                    }
                    testAPI();
                    Locale::setDefault(saveLocale, status);
                }
                break;

        default: name = ""; break;
    }
}




void IntlTestSimpleDateFormatAPI::testAPI()
{
    UErrorCode status = U_ZERO_ERROR;



    logln("Testing SimpleDateFormat constructors");

    SimpleDateFormat def(status);
    if(U_FAILURE(status)) {
        dataerrln("ERROR: Could not create SimpleDateFormat (default) - exitting");
        return;
    }

    status = U_ZERO_ERROR;
    const UnicodeString pattern("yyyy.MM.dd G 'at' hh:mm:ss z", "");
    const UnicodeString override("y=hebr;d=thai;s=arab", ""); 
    const UnicodeString override_bogus("y=hebr;d=thai;s=bogus", "");

    SimpleDateFormat pat(pattern, status);
    if(U_FAILURE(status)) {
       errln("ERROR: Could not create SimpleDateFormat (pattern) - %s", u_errorName(status));
    }

    status = U_ZERO_ERROR;
    SimpleDateFormat pat_fr(pattern, Locale::getFrench(), status);
    if(U_FAILURE(status)) {
        errln("ERROR: Could not create SimpleDateFormat (pattern French)");
    }

    status = U_ZERO_ERROR;
    DateFormatSymbols *symbols = new DateFormatSymbols(Locale::getFrench(), status);
    if(U_FAILURE(status)) {
        errln("ERROR: Could not create DateFormatSymbols (French)");
    }

    status = U_ZERO_ERROR;
    SimpleDateFormat cust1(pattern, symbols, status);
    if(U_FAILURE(status)) {
        dataerrln("ERROR: Could not create SimpleDateFormat (pattern, symbols*) - exitting");
        return;
    }

    status = U_ZERO_ERROR;
    SimpleDateFormat cust2(pattern, *symbols, status);
    if(U_FAILURE(status)) {
        errln("ERROR: Could not create SimpleDateFormat (pattern, symbols)");
    }

    status = U_ZERO_ERROR;
    logln(UnicodeString("Override with: ") + override);
    SimpleDateFormat ovr1(pattern, override, status);
    if(U_FAILURE(status)) {
      errln("ERROR: Could not create SimpleDateFormat (pattern, override) - %s", u_errorName(status));
    }

    status = U_ZERO_ERROR;
    SimpleDateFormat ovr2(pattern, override, Locale::getGerman(), status);
    if(U_FAILURE(status)) {
        errln("ERROR: Could not create SimpleDateFormat (pattern, override, locale) - %s", u_errorName(status));
    }

    status = U_ZERO_ERROR;
    logln(UnicodeString("Override with: ") + override_bogus);
    SimpleDateFormat ovr3(pattern, override_bogus, Locale::getGerman(), status);
    if(U_SUCCESS(status)) {
        errln("ERROR: Should not have been able to create SimpleDateFormat (pattern, override, locale) with a bogus override");
    }


    SimpleDateFormat copy(pat);



    logln("Testing clone(), assignment and equality operators");

    if( ! (copy == pat) || copy != pat) {
        errln("ERROR: Copy constructor (or ==) failed");
    }

    copy = cust1;
    if(copy != cust1) {
        errln("ERROR: Assignment (or !=) failed");
    }

    Format *clone = def.clone();
    if( ! (*clone == def) ) {
        errln("ERROR: Clone() (or ==) failed");
    }
    delete clone;



    logln("Testing various format() methods");

    UDate d = 837039928046.0;
    Formattable fD(d, Formattable::kIsDate);

    UnicodeString res1, res2;
    FieldPosition pos1(0), pos2(0);
    
    res1 = def.format(d, res1, pos1);
    logln( (UnicodeString) "" + d + " formatted to " + res1);

    status = U_ZERO_ERROR;
    res2 = cust1.format(fD, res2, pos2, status);
    if(U_FAILURE(status)) {
        errln("ERROR: format(Formattable [Date]) failed");
    }
    logln((UnicodeString) "" + fD.getDate() + " formatted to " + res2);



    logln("Testing parse()");

    UnicodeString text("02/03/76 2:50 AM, CST");
    UDate result1, result2;
    ParsePosition pos(0);
    result1 = def.parse(text, pos);
    logln(text + " parsed into " + result1);

    status = U_ZERO_ERROR;
    result2 = def.parse(text, status);
    if(U_FAILURE(status)) {
        errln("ERROR: parse() failed");
    }
    logln(text + " parsed into " + result2);



    logln("Testing getters and setters");

    const DateFormatSymbols *syms = pat.getDateFormatSymbols();
    if(!syms) {
      errln("Couldn't obtain DateFormatSymbols. Quitting test!");
      return;
    }
    if(syms->getDynamicClassID() != DateFormatSymbols::getStaticClassID()) {
        errln("ERROR: format->getDateFormatSymbols()->getDynamicClassID() != DateFormatSymbols::getStaticClassID()");
    }
    DateFormatSymbols *newSyms = new DateFormatSymbols(*syms);
    def.adoptDateFormatSymbols(newSyms);    
    pat_fr.setDateFormatSymbols(*newSyms);
    if( *(pat.getDateFormatSymbols()) != *(def.getDateFormatSymbols())) {
        errln("ERROR: adopt or set DateFormatSymbols() failed");
    }

    status = U_ZERO_ERROR;
    UDate startDate = pat.get2DigitYearStart(status);
    if(U_FAILURE(status)) {
        errln("ERROR: getTwoDigitStartDate() failed");
    }
    
    status = U_ZERO_ERROR;
    pat_fr.set2DigitYearStart(startDate, status);
    if(U_FAILURE(status)) {
        errln("ERROR: setTwoDigitStartDate() failed");
    }


    newSyms  =new DateFormatSymbols("gregorian", status);
    if(U_FAILURE(status)) {
        errln("ERROR: new DateFormatSymbols() failed");
    }
    def.adoptDateFormatSymbols(newSyms);



    logln("Testing applyPattern()");

    UnicodeString p1("yyyy.MM.dd G 'at' hh:mm:ss z");
    logln("Applying pattern " + p1);
    status = U_ZERO_ERROR;
    pat.applyPattern(p1);

    UnicodeString s2;
    s2 = pat.toPattern(s2);
    logln("Extracted pattern is " + s2);
    if(s2 != p1) {
        errln("ERROR: toPattern() result did not match pattern applied");
    }

    logln("Applying pattern " + p1);
    status = U_ZERO_ERROR;
    pat.applyLocalizedPattern(p1, status);
    if(U_FAILURE(status)) {
        errln("ERROR: applyPattern() failed with %s", u_errorName(status));
    }
    UnicodeString s3;
    status = U_ZERO_ERROR;
    s3 = pat.toLocalizedPattern(s3, status);
    if(U_FAILURE(status)) {
        errln("ERROR: toLocalizedPattern() failed");
    }
    logln("Extracted pattern is " + s3);
    if(s3 != p1) {
        errln("ERROR: toLocalizedPattern() result did not match pattern applied");
    }



    logln("Testing getStaticClassID()");

    status = U_ZERO_ERROR;
    DateFormat *test = new SimpleDateFormat(status);
    if(U_FAILURE(status)) {
        errln("ERROR: Couldn't create a SimpleDateFormat");
    }

    if(test->getDynamicClassID() != SimpleDateFormat::getStaticClassID()) {
        errln("ERROR: getDynamicClassID() didn't return the expected value");
    }
    
    delete test;
    

    SimpleDateFormat object(UNICODE_STRING_SIMPLE("YYYY'W'wwe"), status);
    if(U_FAILURE(status)) {
        errln("ERROR: Couldn't create a SimpleDateFormat");
    }
    object.setLenient(false);
    ParsePosition pp(0);
    UDate udDate = object.parse("2007W014", pp);
    if ((double)udDate == 0.0) {
        errln("ERROR: Parsing failed using 'Y' and 'e'");
    }


    if (object.getNumberFormatForField('B') != NULL) {
        errln("B is not a valid field, "
              "getNumberFormatForField should return NULL");
    }
}

#endif 
