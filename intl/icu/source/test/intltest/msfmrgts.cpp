




 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "msfmrgts.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/gregocal.h"
#include "putilimp.h"





#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void
MessageFormatRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(Test4074764)
    
    TESTCASE_AUTO(Test4031438)
    TESTCASE_AUTO(Test4052223)
    TESTCASE_AUTO(Test4104976)
    TESTCASE_AUTO(Test4106659)
    TESTCASE_AUTO(Test4106660)
    TESTCASE_AUTO(Test4111739)
    TESTCASE_AUTO(Test4114743)
    TESTCASE_AUTO(Test4116444)
    TESTCASE_AUTO(Test4114739)
    TESTCASE_AUTO(Test4113018)
    TESTCASE_AUTO(Test4106661)
    TESTCASE_AUTO(Test4094906)
    TESTCASE_AUTO(Test4118592)
    TESTCASE_AUTO(Test4118594)
    TESTCASE_AUTO(Test4105380)
    TESTCASE_AUTO(Test4120552)
    TESTCASE_AUTO(Test4142938)
    TESTCASE_AUTO(TestChoicePatternQuote)
    TESTCASE_AUTO(Test4112104)
    TESTCASE_AUTO(TestAPI)
    TESTCASE_AUTO_END;
}

UBool 
MessageFormatRegressionTest::failure(UErrorCode status, const char* msg, UBool possibleDataError)
{
    if(U_FAILURE(status)) {
        if (possibleDataError) {
            dataerrln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        } else {
            errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        }
        return TRUE;
    }

    return FALSE;
}





void MessageFormatRegressionTest::Test4074764() {
    UnicodeString pattern [] = {
        "Message without param",
        "Message with param:{0}",
        "Longer Message with param {0}"
    };
    
    
    
    

    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *messageFormatter = new MessageFormat("", status);

    failure(status, "couldn't create MessageFormat");

    
        
        messageFormatter->applyPattern(pattern[1], status);
        failure(status, "messageFormat->applyPattern");
        
        Formattable params [] = {
            Formattable(UnicodeString("BUG")), 
            Formattable(0, Formattable::kIsDate)
        };
        UnicodeString tempBuffer;
        FieldPosition pos(FieldPosition::DONT_CARE);
        tempBuffer = messageFormatter->format(params, 2, tempBuffer, pos, status);
        if( tempBuffer != "Message with param:BUG" || failure(status, "messageFormat->format"))
            errln("MessageFormat with one param test failed.");
        logln("Formatted with one extra param : " + tempBuffer);

        
        messageFormatter->applyPattern(pattern[0], status);
        failure(status, "messageFormatter->applyPattern");
        
        
        
        
        
        
        
        
        
        tempBuffer.remove();
        tempBuffer = messageFormatter->format(NULL, 0, tempBuffer, pos, status);

        if( tempBuffer != "Message without param" || failure(status, "messageFormat->format"))
            errln("MessageFormat with no param test failed.");
        logln("Formatted with no params : " + tempBuffer);

        tempBuffer.remove();
        tempBuffer = messageFormatter->format(params, 2, tempBuffer, pos, status);
         if (tempBuffer != "Message without param" || failure(status, "messageFormat->format"))
            errln("Formatted with arguments > subsitution failed. result = " + tempBuffer);
         logln("Formatted with extra params : " + tempBuffer);
        
        
        
        
        
        
        
        
    



    delete messageFormatter;
}





























void MessageFormatRegressionTest::Test4031438() 
{
    UErrorCode status = U_ZERO_ERROR;
    
    UnicodeString pattern1("Impossible {1} has occurred -- status code is {0} and message is {2}.");
    UnicodeString pattern2("Double '' Quotes {0} test and quoted '{1}' test plus 'other {2} stuff'.");

    MessageFormat *messageFormatter = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    
    const UBool possibleDataError = TRUE;

    
        logln("Apply with pattern : " + pattern1);
        messageFormatter->applyPattern(pattern1, status);
        failure(status, "messageFormat->applyPattern");
        
        Formattable params []= {
            Formattable((int32_t)7)
        };
        UnicodeString tempBuffer;
        FieldPosition pos(FieldPosition::DONT_CARE);
        tempBuffer = messageFormatter->format(params, 1, tempBuffer, pos, status);
        if(tempBuffer != "Impossible {1} has occurred -- status code is 7 and message is {2}." || failure(status, "MessageFormat::format"))
            dataerrln("Tests arguments < substitution failed");
        logln("Formatted with 7 : " + tempBuffer);
        ParsePosition pp(0);
        int32_t count = 0;
        Formattable *objs = messageFormatter->parse(tempBuffer, pp, count);
        
        

        NumberFormat *fmt = 0;
        UnicodeString temp, temp1;
        
        for (int i = 0; i < count; i++) {
            
            
            Formattable obj = objs[i];
            temp.remove();
            if(obj.getType() == Formattable::kString)
                temp = obj.getString(temp);
            else {
                fmt = NumberFormat::createInstance(status);
                switch (obj.getType()) {
                case Formattable::kLong: fmt->format(obj.getLong(), temp); break;
                case Formattable::kInt64: fmt->format(obj.getInt64(), temp); break;
                case Formattable::kDouble: fmt->format(obj.getDouble(), temp); break;
                default: break;
                }
            }

            
            Formattable obj1 = params[i];
            temp1.remove();
            if(obj1.getType() == Formattable::kString)
                temp1 = obj1.getString(temp1);
            else {
                fmt = NumberFormat::createInstance(status);
                switch (obj1.getType()) {
                case Formattable::kLong: fmt->format(obj1.getLong(), temp1); break;
                case Formattable::kInt64: fmt->format(obj1.getInt64(), temp1); break;
                case Formattable::kDouble: fmt->format(obj1.getDouble(), temp1); break;
                default: break;
                }
            }

            
            if (temp != temp1) {
                errln("Parse failed on object " + objs[i].getString(temp1) + " at index : " + i);
            }       
        }

        delete fmt;
        delete [] objs;

        
        

        



        logln("Apply with pattern : " + pattern2);
        messageFormatter->applyPattern(pattern2, status);
        failure(status, "messageFormatter->applyPattern", possibleDataError);
        tempBuffer.remove();
        tempBuffer = messageFormatter->format(params, 1, tempBuffer, pos, status);
        if (tempBuffer != "Double ' Quotes 7 test and quoted {1} test plus 'other {2} stuff'.")
            dataerrln("quote format test (w/ params) failed. - %s", u_errorName(status));
        logln("Formatted with params : " + tempBuffer);
        
        




    


        delete messageFormatter;
}

void MessageFormatRegressionTest::Test4052223()
{

    ParsePosition pos(0);
    if (pos.getErrorIndex() != -1) {
        errln("ParsePosition.getErrorIndex initialization failed.");
    }

    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *fmt = new MessageFormat("There are {0} apples growing on the {1} tree.", status);
    failure(status, "new MessageFormat");
    UnicodeString str("There is one apple growing on the peach tree.");
    
    int32_t count = 0;
    fmt->parse(str, pos, count);

    logln(UnicodeString("unparsable string , should fail at ") + pos.getErrorIndex());
    if (pos.getErrorIndex() == -1)
        errln("Bug 4052223 failed : parsing string " + str);
    pos.setErrorIndex(4);
    if (pos.getErrorIndex() != 4)
        errln(UnicodeString("setErrorIndex failed, got ") + pos.getErrorIndex() + " instead of 4");
    
    ChoiceFormat *f = new ChoiceFormat(
        "-1#are negative|0#are no or fraction|1#is one|1.0<is 1+|2#are two|2<are more than 2.", status);
    failure(status, "new ChoiceFormat");
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    Formattable obj;
    f->parse("are negative", obj, pos);
    if (pos.getErrorIndex() != -1 && obj.getDouble() == -1.0)
        errln(UnicodeString("Parse with \"are negative\" failed, at ") + pos.getErrorIndex());
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    f->parse("are no or fraction ", obj, pos);
    if (pos.getErrorIndex() != -1 && obj.getDouble() == 0.0)
        errln(UnicodeString("Parse with \"are no or fraction\" failed, at ") + pos.getErrorIndex());
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    f->parse("go postal", obj, pos);
    if (pos.getErrorIndex() == -1 && ! uprv_isNaN(obj.getDouble()))
        errln(UnicodeString("Parse with \"go postal\" failed, at ") + pos.getErrorIndex());
    
    delete fmt;
    delete f;
}






void MessageFormatRegressionTest::Test4104976()
{
    double limits [] = {1, 20};
    UnicodeString formats [] = {
        UnicodeString("xyz"), 
        UnicodeString("abc")
    };
    int32_t formats_length = (int32_t)(sizeof(formats)/sizeof(formats[0]));
    UErrorCode status = U_ZERO_ERROR;
    ChoiceFormat *cf = new ChoiceFormat(limits, formats, formats_length);
    failure(status, "new ChoiceFormat");
    
        log("Compares to null is always false, returned : ");
        logln(cf == NULL ? "TRUE" : "FALSE");
    



    delete cf;
}








void MessageFormatRegressionTest::Test4106659()
{
    


















}






void MessageFormatRegressionTest::Test4106660()
{
    double limits [] = {3, 1, 2};
    UnicodeString formats [] = {
        UnicodeString("Three"), 
            UnicodeString("One"), 
            UnicodeString("Two")
    };
    ChoiceFormat *cf = new ChoiceFormat(limits, formats, 3);
    double d = 5.0;
    UnicodeString str;
    FieldPosition pos(FieldPosition::DONT_CARE);
    str = cf->format(d, str, pos);
    if (str != "Two")
        errln( (UnicodeString) "format(" + d + ") = " + str);

    delete cf;
}







void MessageFormatRegressionTest::Test4111739()
{
    

































}



void MessageFormatRegressionTest::Test4114743()
{
    UnicodeString originalPattern("initial pattern");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(originalPattern, status);
    failure(status, "new MessageFormat");
    
        UnicodeString illegalPattern("ab { '}' de");
        mf->applyPattern(illegalPattern, status);
        if( ! U_FAILURE(status))
            errln("illegal pattern: \"" + illegalPattern + "\"");
    



    delete mf;
}




void MessageFormatRegressionTest::Test4116444()
{
    UnicodeString patterns [] = {
        (UnicodeString)"", 
        (UnicodeString)"one", 
        (UnicodeString) "{0,date,short}"
    };
    
    UErrorCode status = U_ZERO_ERROR;    
    MessageFormat *mf = new MessageFormat("", status);
    failure(status, "new MessageFormat");

    for (int i = 0; i < 3; i++) {
        UnicodeString pattern = patterns[i];
        mf->applyPattern(pattern, status);
        failure(status, "mf->applyPattern", TRUE);

        
        int32_t count = 0;    
        ParsePosition pp(0);
        Formattable *array = mf->parse(UnicodeString(""), pp, count);
            logln("pattern: \"" + pattern + "\"");
            log(" parsedObjects: ");
            if (array != NULL) {
                log("{");
                for (int j = 0; j < count; j++) {
                    
                    UnicodeString dummy;
                    dataerrln("\"" + array[j].getString(dummy) + "\"");
                    
                     
                    if (j < count- 1) 
                        log(",");
                }
                log("}") ;
                delete[] array;
            } else {
                log("null");
            }
            logln("");
        



    }

    delete mf;
}





void MessageFormatRegressionTest::Test4114739()
{

    UErrorCode status = U_ZERO_ERROR;    
    MessageFormat *mf = new MessageFormat("<{0}>", status);
    failure(status, "new MessageFormat");

    Formattable *objs1 = NULL;
    
    
    
    UnicodeString pat;
    UnicodeString res;
        logln("pattern: \"" + mf->toPattern(pat) + "\"");
        log("format(null) : ");
        FieldPosition pos(FieldPosition::DONT_CARE);
        logln("\"" + mf->format(objs1, 0, res, pos, status) + "\"");
        failure(status, "mf->format");
        





    



    delete mf;
}




void MessageFormatRegressionTest::Test4113018()
{
    UnicodeString originalPattern("initial pattern");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(originalPattern, status);
    failure(status, "new messageFormat");
    UnicodeString illegalPattern("format: {0, xxxYYY}");
    UnicodeString pat;
    logln("pattern before: \"" + mf->toPattern(pat) + "\"");
    logln("illegal pattern: \"" + illegalPattern + "\"");
    
        mf->applyPattern(illegalPattern, status);
        if( ! U_FAILURE(status))
            errln("Should have thrown IllegalArgumentException for pattern : " + illegalPattern);
    



    delete mf;
}




void MessageFormatRegressionTest::Test4106661()
{
    UErrorCode status = U_ZERO_ERROR;
    ChoiceFormat *fmt = new ChoiceFormat(
      "-1#are negative| 0#are no or fraction | 1#is one |1.0<is 1+ |2#are two |2<are more than 2.", status);
    failure(status, "new ChoiceFormat");
    UnicodeString pat;
    logln("Formatter Pattern : " + fmt->toPattern(pat));

    FieldPosition bogus(FieldPosition::DONT_CARE);
    UnicodeString str;

    
    logln("Format with -INF : " + fmt->format(Formattable(-uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0 : " + fmt->format(Formattable((int32_t)0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0.9 : " + fmt->format(Formattable(0.9), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.0 : " + fmt->format(Formattable(1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.5 : " + fmt->format(Formattable(1.5), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2 : " + fmt->format(Formattable((int32_t)2), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2.1 : " + fmt->format(Formattable(2.1), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with NaN : " + fmt->format(Formattable(uprv_getNaN()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with +INF : " + fmt->format(Formattable(uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");

    delete fmt;
}




void MessageFormatRegressionTest::Test4094906()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern("-");
    pattern += (UChar) 0x221E;
    pattern += "<are negative|0<are no or fraction|1#is one|1<is 1+|";
    pattern += (UChar) 0x221E;
    pattern += "<are many.";

    ChoiceFormat *fmt = new ChoiceFormat(pattern, status);
    failure(status, "new ChoiceFormat");
    UnicodeString pat;
    if (fmt->toPattern(pat) != pattern) {
        errln( (UnicodeString) "Formatter Pattern : " + pat);
        errln( (UnicodeString) "Expected Pattern  : " + pattern);
    }
    FieldPosition bogus(FieldPosition::DONT_CARE);
    UnicodeString str;

    
    logln("Format with -INF : " + fmt->format(Formattable(-uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0 : " + fmt->format(Formattable((int32_t)0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0.9 : " + fmt->format(Formattable(0.9), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.0 : " + fmt->format(Formattable(1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.5 : " + fmt->format(Formattable(1.5), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2 : " + fmt->format(Formattable((int32_t)2), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2.1 : " + fmt->format(Formattable(2.1), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with NaN : " + fmt->format(Formattable(uprv_getNaN()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with +INF : " + fmt->format(Formattable(uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");

    delete fmt;
}




void MessageFormatRegressionTest::Test4118592()
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat("", status);
    failure(status, "new messageFormat");
    UnicodeString pattern("{0,choice,1#YES|2#NO}");
    UnicodeString prefix("");
    Formattable *objs = 0;

    for (int i = 0; i < 5; i++) {
        UnicodeString formatted;
        formatted = prefix + "YES";
        mf->applyPattern(prefix + pattern, status);
        failure(status, "mf->applyPattern");
        prefix += "x";
        
        int32_t count = 0;
        ParsePosition pp(0);
        objs = mf->parse(formatted, pp, count);
        UnicodeString pat;
        logln(UnicodeString("") + i + ". pattern :\"" + mf->toPattern(pat) + "\"");
        log(" \"" + formatted + "\" parsed as ");
        if (objs == NULL) 
            logln("  null");
        else {
            UnicodeString temp;
            if(objs[0].getType() == Formattable::kString)
                logln((UnicodeString)"  " + objs[0].getString(temp));
            else
                logln((UnicodeString)"  " + (objs[0].getType() == Formattable::kLong ? objs[0].getLong() : objs[0].getDouble()));
            delete[] objs;

        }
    }

    delete mf;
}



void MessageFormatRegressionTest::Test4118594()
{
    UErrorCode status = U_ZERO_ERROR;
    const UBool possibleDataError = TRUE;
    MessageFormat *mf = new MessageFormat("{0}, {0}, {0}", status);
    failure(status, "new MessageFormat");
    UnicodeString forParsing("x, y, z");
    
    int32_t count = 0;
    ParsePosition pp(0);
    Formattable *objs = mf->parse(forParsing, pp, count);
    UnicodeString pat;
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    logln("text for parsing: \"" + forParsing + "\"");
    UnicodeString str;
    if (objs[0].getString(str) != "z")
        errln("argument0: \"" + objs[0].getString(str) + "\"");
    mf->applyPattern("{0,number,#.##}, {0,number,#.#}", status);
    failure(status, "mf->applyPattern", possibleDataError);
    
    Formattable oldobjs [] = {Formattable(3.1415)};
    UnicodeString result;
    FieldPosition pos(FieldPosition::DONT_CARE);
    result = mf->format( oldobjs, 1, result, pos, status );
    failure(status, "mf->format", possibleDataError);
    pat.remove();
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    logln("text for parsing: \"" + result + "\"");
    
    if (result != "3.14, 3.1")
        dataerrln("result = " + result + " - " + u_errorName(status));
    
    int32_t count1 = 0;
    pp.setIndex(0);
    Formattable *newobjs = mf->parse(result, pp, count1);
    
    if (newobjs == NULL) {
        dataerrln("Error calling MessageFormat::parse");
    } else {
        if (newobjs[0].getDouble() != 3.1)
            errln( UnicodeString("newobjs[0] = ") + newobjs[0].getDouble());
    }

    delete [] objs;
    delete [] newobjs;
    delete mf;
}



void MessageFormatRegressionTest::Test4105380()
{
    UnicodeString patternText1("The disk \"{1}\" contains {0}.");
    UnicodeString patternText2("There are {0} on the disk \"{1}\"");
    UErrorCode status = U_ZERO_ERROR;
    const UBool possibleDataError = TRUE;
    MessageFormat *form1 = new MessageFormat(patternText1, status);
    failure(status, "new MessageFormat");
    MessageFormat *form2 = new MessageFormat(patternText2, status);
    failure(status, "new MessageFormat");
    double filelimits [] = {0,1,2};
    UnicodeString filepart [] = {
        (UnicodeString)"no files",
            (UnicodeString)"one file",
            (UnicodeString)"{0,number} files"
    };
    ChoiceFormat *fileform = new ChoiceFormat(filelimits, filepart, 3);
    form1->setFormat(1, *fileform);
    form2->setFormat(0, *fileform);
    
    Formattable testArgs [] = {
        Formattable((int32_t)12373), 
            Formattable((UnicodeString)"MyDisk")
    };
    
    FieldPosition bogus(FieldPosition::DONT_CARE);

    UnicodeString result;
    logln(form1->format(testArgs, 2, result, bogus, status));
    failure(status, "form1->format", possibleDataError);
    result.remove();
    logln(form2->format(testArgs, 2, result, bogus, status));
    failure(status, "form1->format", possibleDataError);

    delete form1;
    delete form2;
    delete fileform;
}



void MessageFormatRegressionTest::Test4120552()
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat("pattern", status);
    failure(status, "new MessageFormat");
    UnicodeString texts[] = {
        (UnicodeString)"pattern", 
            (UnicodeString)"pat", 
            (UnicodeString)"1234"
    };
    UnicodeString pat;
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    for (int i = 0; i < 3; i++) {
        ParsePosition pp(0);
        
        int32_t count = 0;
        Formattable *objs = mf->parse(texts[i], pp, count);
        log("  text for parsing: \"" + texts[i] + "\"");
        if (objs == NULL) {
            logln("  (incorrectly formatted string)");
            if (pp.getErrorIndex() == -1)
                errln(UnicodeString("Incorrect error index: ") + pp.getErrorIndex());
        } else {
            logln("  (correctly formatted string)");
            delete[] objs;
        }
    }
    delete mf;
}







void MessageFormatRegressionTest::Test4142938() 
{
    UnicodeString pat = CharsToUnicodeString("''Vous'' {0,choice,0#n''|1#}avez s\\u00E9lectionn\\u00E9 "
        "{0,choice,0#aucun|1#{0}} client{0,choice,0#s|1#|2#s} "
        "personnel{0,choice,0#s|1#|2#s}.");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(pat, status);
    failure(status, "new MessageFormat");

    UnicodeString PREFIX [] = {
        CharsToUnicodeString("'Vous' n'avez s\\u00E9lectionn\\u00E9 aucun clients personnels."),
        CharsToUnicodeString("'Vous' avez s\\u00E9lectionn\\u00E9 "),
        CharsToUnicodeString("'Vous' avez s\\u00E9lectionn\\u00E9 ")
    };  
    UnicodeString SUFFIX [] = {
        UnicodeString(),
        UNICODE_STRING(" client personnel.", 18),
        UNICODE_STRING(" clients personnels.", 20)
    };

    for (int i=0; i<3; i++) {
        UnicodeString out;
        
        Formattable objs [] = {
            Formattable((int32_t)i)
        };
        FieldPosition pos(FieldPosition::DONT_CARE);
        out = mf->format(objs, 1, out, pos, status);
        if (!failure(status, "mf->format", TRUE)) {
            if (SUFFIX[i] == "") {
                if (out != PREFIX[i])
                    errln((UnicodeString)"" + i + ": Got \"" + out + "\"; Want \"" + PREFIX[i] + "\"");
            }
            else {
                if (!out.startsWith(PREFIX[i]) ||
                    !out.endsWith(SUFFIX[i]))
                    errln((UnicodeString)"" + i + ": Got \"" + out + "\"; Want \"" + PREFIX[i] + "\"...\"" +
                          SUFFIX[i] + "\"");
            }
        }
    }

    delete mf;
}









void MessageFormatRegressionTest::TestChoicePatternQuote() 
{
    
    
    
    
    
    
    
    
    
    UnicodeString DATA [] = {
        
        
        "0#can't|1#can",            "can't",          "can",
        "0#pound(#)='#''|1#xyz",    "pound(#)='#''",  "xyz",
        "0#1<2 '| 1=1'|1#'",        "1<2 '| 1=1'",    "'",
    };
    for (int i=0; i<9; i+=3) {
        
            UErrorCode status = U_ZERO_ERROR;
            ChoiceFormat *cf = new ChoiceFormat(DATA[i], status);
            failure(status, "new ChoiceFormat");
            for (int j=0; j<=1; ++j) {
                UnicodeString out;
                FieldPosition pos(FieldPosition::DONT_CARE);
                out = cf->format((double)j, out, pos);
                if (out != DATA[i+1+j])
                    errln("Fail: Pattern \"" + DATA[i] + "\" x "+j+" -> " +
                          out + "; want \"" + DATA[i+1+j] + "\"");
            }
            UnicodeString pat;
            pat = cf->toPattern(pat);
            UnicodeString pat2;
            ChoiceFormat *cf2 = new ChoiceFormat(pat, status);
            pat2 = cf2->toPattern(pat2);
            if (pat != pat2)
                errln("Fail: Pattern \"" + DATA[i] + "\" x toPattern -> \"" + pat + "\"");
            else
                logln("Ok: Pattern \"" + DATA[i] + "\" x toPattern -> \"" + pat + "\"");
        



    
        delete cf;
        delete cf2;
    }
}






void MessageFormatRegressionTest::Test4112104() 
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *format = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    
        
        if (format == NULL) {
            
            errln("MessageFormat.equals(null) returns false");
        }
    



    delete format;
}

void MessageFormatRegressionTest::TestAPI() {
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *format = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    
    
    MessageFormat *fmt = new MessageFormat("",status);
    format->adoptFormat("some_name",fmt,status);  
    failure(status, "adoptFormat");

    
    format->setFormat((int32_t)0,*fmt);
    format->getFormat("some_other_name",status);  
    failure(status, "getFormat");
    delete format;
}

#endif 
