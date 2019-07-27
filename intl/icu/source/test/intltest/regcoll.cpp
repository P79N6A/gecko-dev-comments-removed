





#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/localpointer.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "regcoll.h"
#include "sfwdchit.h"
#include "testutil.h"
#include "cmemory.h"

#define ARRAY_LENGTH(array) ((int32_t)(sizeof array / sizeof array[0]))

CollationRegressionTest::CollationRegressionTest()
{
    UErrorCode status = U_ZERO_ERROR;

    en_us = (RuleBasedCollator *)Collator::createInstance(Locale::getUS(), status);
    if(U_FAILURE(status)) {
      delete en_us;
      en_us = 0;
      errcheckln(status, "Collator creation failed with %s", u_errorName(status));
      return;
    }
}

CollationRegressionTest::~CollationRegressionTest()
{
    delete en_us;
}


    



void CollationRegressionTest::Test4048446()
{
    const UnicodeString test1 = "XFILE What subset of all possible test cases has the highest probability of detecting the most errors?";
    const UnicodeString test2 = "Xf_ile What subset of all possible test cases has the lowest probability of detecting the least errors?";
    CollationElementIterator *i1 = en_us->createCollationElementIterator(test1);
    CollationElementIterator *i2 = en_us->createCollationElementIterator(test1);
    UErrorCode status = U_ZERO_ERROR;

    if (i1 == NULL|| i2 == NULL)
    {
        errln("Could not create CollationElementIterator's");
        delete i1;
        delete i2;
        return;
    }

    while (i1->next(status) != CollationElementIterator::NULLORDER)
    {
        if (U_FAILURE(status))
        {
            errln("error calling next()");

            delete i1;
            delete i2;
            return;
        }
    }

    i1->reset();

    assertEqual(*i1, *i2);

    delete i1;
    delete i2;
}





void CollationRegressionTest::Test4051866()
{
    UnicodeString rules;
    UErrorCode status = U_ZERO_ERROR;

    rules += "&n < o ";
    rules += "& oe ,o";
    rules += (UChar)0x3080;
    rules += "& oe ,";
    rules += (UChar)0x1530;
    rules += " ,O";
    rules += "& OE ,O";
    rules += (UChar)0x3080;
    rules += "& OE ,";
    rules += (UChar)0x1520;
    rules += "< p ,P";

    
    LocalPointer<RuleBasedCollator> c1(new RuleBasedCollator(rules, status), status);
    if (U_FAILURE(status)) {
        errln("RuleBasedCollator(rule string) failed - %s", u_errorName(status));
        return;
    }

    
    LocalPointer<RuleBasedCollator> c2(new RuleBasedCollator(c1->getRules(), status), status);
    if (U_FAILURE(status)) {
        errln("RuleBasedCollator(rule string from other RBC) failed - %s", u_errorName(status));
        return;
    }

    
    if (!(c1->getRules() == c2->getRules()))
    {
        errln("Rules are not equal");
    }
}





void CollationRegressionTest::Test4053636()
{
    if (en_us->equals("black_bird", "black"))
    {
        errln("black-bird == black");
    }
}






void CollationRegressionTest::Test4054238()
{
    const UChar chars3[] = {0x61, 0x00FC, 0x62, 0x65, 0x63, 0x6b, 0x20, 0x47, 0x72, 0x00F6, 0x00DF, 0x65, 0x20, 0x4c, 0x00FC, 0x62, 0x63, 0x6b, 0};
    const UnicodeString test3(chars3);
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    
    
    UErrorCode status = U_ZERO_ERROR;
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    CollationElementIterator *i1 = c->createCollationElementIterator(test3);
    delete i1;
    delete c;
}





void CollationRegressionTest::Test4054734()
{
    














    static const UChar decomp[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x0001, 0},      {0x3c, 0}, {0x0002, 0},
        {0x0001, 0},      {0x3d, 0}, {0x0001, 0},
        {0x41, 0x0001, 0}, {0x3e, 0}, {0x7e, 0x0002, 0},
        {0x00c0, 0},      {0x3d, 0}, {0x41, 0x0300, 0}
    };


    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    c->setStrength(Collator::IDENTICAL);

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, decomp, ARRAY_LENGTH(decomp));

    delete c;
}





void CollationRegressionTest::Test4054736()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    c->setStrength(Collator::SECONDARY);
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0xFB4F, 0}, {0x3d, 0}, {0x05D0, 0x05DC}  
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}





void CollationRegressionTest::Test4058613()
{
    
    

    Locale oldDefault = Locale::getDefault();
    UErrorCode status = U_ZERO_ERROR;

    Locale::setDefault(Locale::getKorean(), status);

    if (U_FAILURE(status))
    {
        errln("Could not set default locale to Locale::KOREAN");
        return;
    }

    Collator *c = NULL;

    c = Collator::createInstance("en_US", status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Could not create a Korean collator");
        Locale::setDefault(oldDefault, status);
        delete c;
        return;
    }

    
    
    if (c->getAttribute(UCOL_NORMALIZATION_MODE, status) != UCOL_OFF)
    {
      errln("Decomposition is not set to NO_DECOMPOSITION for Korean collator");
    }

    delete c;

    Locale::setDefault(oldDefault, status);
}






void CollationRegressionTest::Test4059820()
{
    UErrorCode status = U_ZERO_ERROR;

    RuleBasedCollator *c = NULL;
    UnicodeString rules = "&9 < a < b , c/a < d < z";

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failure building a collator.");
        delete c;
        return;
    }

    if ( c->getRules().indexOf("c/a") == -1)
    {
        errln("returned rules do not contain 'c/a'");
    }

    delete c;
}





void CollationRegressionTest::Test4060154()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString rules;

    rules += "&f < g, G < h, H < i, I < j, J";
    rules +=  " & H < ";
    rules += (UChar)0x0131;
    rules += ", ";
    rules += (UChar)0x0130;
    rules += ", i, I";

    RuleBasedCollator *c = NULL;

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("failure building collator.");
        delete c;
        return;
    }

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

 










    static const UChar tertiary[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x41, 0},    {0x3c, 0}, {0x42, 0},
        {0x48, 0},    {0x3c, 0}, {0x0131, 0},
        {0x48, 0},    {0x3c, 0}, {0x49, 0},
        {0x0131, 0}, {0x3c, 0}, {0x0130, 0},
        {0x0130, 0}, {0x3c, 0}, {0x69, 0},
        {0x0130, 0}, {0x3e, 0}, {0x48, 0}
    };

    c->setStrength(Collator::TERTIARY);
    compareArray(*c, tertiary, ARRAY_LENGTH(tertiary));

    





    static const UChar secondary[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x48, 0},    {0x3c, 0}, {0x49, 0},
        {0x0131, 0}, {0x3d, 0}, {0x0130, 0}
    };

    c->setStrength(Collator::PRIMARY);
    compareArray(*c, secondary, ARRAY_LENGTH(secondary));

    delete c;
}





void CollationRegressionTest::Test4062418()
{
    UErrorCode status = U_ZERO_ERROR;

    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *) Collator::createInstance(Locale::getCanadaFrench(), status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);






    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x70, 0x00EA, 0x63, 0x68, 0x65, 0}, {0x3c, 0}, {0x70, 0x00E9, 0x63, 0x68, 0x00E9, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}





void CollationRegressionTest::Test4065540()
{
    if (en_us->compare("abcd e", "abcd f") == 0)
    {
        errln("'abcd e' == 'abcd f'");
    }
}







void CollationRegressionTest::Test4066189()
{
    static const UChar chars1[] = {0x1EB1, 0};
    static const UChar chars2[] = {0x61, 0x0306, 0x0300, 0};
    const UnicodeString test1(chars1);
    const UnicodeString test2(chars2);
    UErrorCode status = U_ZERO_ERROR;

    
    
    
    RuleBasedCollator *c1 = (RuleBasedCollator *) en_us->clone();
    c1->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    CollationElementIterator *i1 = c1->createCollationElementIterator(test1);

    RuleBasedCollator *c2 = (RuleBasedCollator *) en_us->clone();
    c2->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    CollationElementIterator *i2 = c2->createCollationElementIterator(test2);

    assertEqual(*i1, *i2);

    delete i2;
    delete c2;
    delete i1;
    delete c1;
}





void CollationRegressionTest::Test4066696()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *)Collator::createInstance(Locale::getCanadaFrench(), status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failure creating collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);














    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00E0, 0}, {0x3e, 0}, {0x01FA, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}





void CollationRegressionTest::Test4076676()
{
    
    
    static const UChar s1[] = {0x41, 0x0301, 0x0302, 0x0300, 0};
    static const UChar s2[] = {0x41, 0x0302, 0x0300, 0x0301, 0};

    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    if (c->compare(s1,s2) == 0)
    {
        errln("Same-class combining chars were reordered");
    }

    delete c;
}





void CollationRegressionTest::Test4079231()
{
    
    
    
    
    
    
    
    
    

 









}





void CollationRegressionTest::Test4078588()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *rbc = new RuleBasedCollator("&9 < a < bb", status);

    if (rbc == NULL || U_FAILURE(status))
    {
        errln("Failed to create RuleBasedCollator.");
        delete rbc;
        return;
    }

    Collator::EComparisonResult result = rbc->compare("a","bb");

    if (result != Collator::LESS)
    {
        errln((UnicodeString)"Compare(a,bb) returned " + (int)result
            + (UnicodeString)"; expected -1");
    }

    delete rbc;
}





void CollationRegressionTest::Test4081866()
{
    
    
    static const UChar s1[] = {0x41, 0x0300, 0x0316, 0x0327, 0x0315, 0};
    static const UChar s2[] = {0x41, 0x0327, 0x0316, 0x0315, 0x0300, 0};

    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    
    
    
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    if (c->compare(s1,s2) != 0)
    {
        errln("Combining chars were not reordered");
    }

    delete c;
}





void CollationRegressionTest::Test4087241()
{
    UErrorCode status = U_ZERO_ERROR;
    Locale da_DK("da", "DK");
    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *) Collator::createInstance(da_DK, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for da_DK locale");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x7a, 0},          {0x3c, 0}, {0x00E6, 0},            
        {0x61, 0x0308, 0},  {0x3c, 0}, {0x61, 0x030A, 0},      
        {0x59, 0},          {0x3c, 0}, {0x75, 0x0308, 0},      
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}





void CollationRegressionTest::Test4087243()
{
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x31, 0x32, 0x33, 0}, {0x3d, 0}, {0x31, 0x32, 0x33, 0x0001, 0}    
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}






void CollationRegressionTest::Test4092260()
{
    UErrorCode status = U_ZERO_ERROR;
    Locale el("el", "");
    Collator *c = NULL;

    c = Collator::createInstance(el, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for el locale.");
        delete c;
        return;
    }

    
    c->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00B5, 0}, {0x3d, 0}, {0x03BC, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}



void CollationRegressionTest::Test4095316()
{
    UErrorCode status = U_ZERO_ERROR;
    Locale el_GR("el", "GR");
    Collator *c = Collator::createInstance(el_GR, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for el_GR locale");
        delete c;
        return;
    }
    
    
    c->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x03D4, 0}, {0x3d, 0}, {0x03AB, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}



void CollationRegressionTest::Test4101940()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = NULL;
    UnicodeString rules = "&9 < a < b";
    UnicodeString nothing = "";

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create RuleBasedCollator");
        delete c;
        return;
    }

    CollationElementIterator *i = c->createCollationElementIterator(nothing);
    i->reset();

    if (i->next(status) != CollationElementIterator::NULLORDER)
    {
        errln("next did not return NULLORDER");
    }

    delete i;
    delete c;
}





void CollationRegressionTest::Test4103436()
{
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x66, 0x69, 0x6c, 0x65, 0}, {0x3c, 0}, {0x66, 0x69, 0x6c, 0x65, 0x20, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0},
        {0x66, 0x69, 0x6c, 0x65, 0}, {0x3c, 0}, {0x66, 0x69, 0x6c, 0x65, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}





void CollationRegressionTest::Test4114076()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    
    
    
    
    
    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0xd4db, 0}, {0x3d, 0}, {0x1111, 0x1171, 0x11b6, 0}
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, test1, ARRAY_LENGTH(test1));

    
    
    
    
    












    delete c;
}






void CollationRegressionTest::Test4124632()
{
    UErrorCode status = U_ZERO_ERROR;
    Collator *coll = NULL;

    coll = Collator::createInstance(Locale::getJapan(), status);

    if (coll == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for Locale::JAPAN");
        delete coll;
        return;
    }

    static const UChar test[] = {0x41, 0x0308, 0x62, 0x63, 0};
    CollationKey key;

    coll->getCollationKey(test, key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("CollationKey creation failed.");
    }

    delete coll;
}





void CollationRegressionTest::Test4132736()
{
    UErrorCode status = U_ZERO_ERROR;

    Collator *c = NULL;

    c = Collator::createInstance(Locale::getCanadaFrench(), status);
    c->setStrength(Collator::TERTIARY);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create a collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x65, 0x0300, 0x65, 0x0301, 0}, {0x3c, 0}, {0x65, 0x0301, 0x65, 0x0300, 0},
        {0x65, 0x0300, 0x0301, 0},       {0x3c, 0}, {0x65, 0x0301, 0x0300, 0}
    };

    compareArray(*c, test1, ARRAY_LENGTH(test1));

    delete c;
}





void CollationRegressionTest::Test4133509()
{
    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x45, 0x78, 0x63, 0x65, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0}, {0x3c, 0}, {0x45, 0x78, 0x63, 0x65, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x49, 0x6e, 0x49, 0x6e, 0x69, 0x74, 0x69, 0x61, 0x6c, 0x69, 0x7a, 0x65, 0x72, 0x45, 0x72, 0x72, 0x6f, 0x72, 0},
        {0x47, 0x72, 0x61, 0x70, 0x68, 0x69, 0x63, 0x73, 0},      {0x3c, 0}, {0x47, 0x72, 0x61, 0x70, 0x68, 0x69, 0x63, 0x73, 0x45, 0x6e, 0x76, 0x69, 0x72, 0x6f, 0x6e, 0x6d, 0x65, 0x6e, 0x74, 0},
        {0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0},                  {0x3c, 0}, {0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x42, 0x75, 0x66, 0x66, 0x65, 0x72, 0}
    };

    compareArray(*en_us, test1, ARRAY_LENGTH(test1));
}





void CollationRegressionTest::Test4114077()
{
    
    

    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00C0, 0},                     {0x3d, 0}, {0x41, 0x0300, 0},            
        {0x70, 0x00ea, 0x63, 0x68, 0x65, 0}, {0x3e, 0}, {0x70, 0x00e9, 0x63, 0x68, 0x00e9, 0},
        {0x0204, 0},                     {0x3d, 0}, {0x45, 0x030F, 0},
        {0x01fa, 0},                     {0x3d, 0}, {0x41, 0x030a, 0x0301, 0},    
                                                
        {0x41, 0x0300, 0x0316, 0},         {0x3c, 0}, {0x41, 0x0316, 0x0300, 0}        
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    compareArray(*c, test1, ARRAY_LENGTH(test1));

    static const UChar test2[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x41, 0x0300, 0x0316, 0}, {0x3d, 0}, {0x41, 0x0316, 0x0300, 0}      
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, test2, ARRAY_LENGTH(test2));

    delete c;
}





void CollationRegressionTest::Test4141640()
{
    
    
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    int32_t i, localeCount;
    const Locale *locales = Locale::getAvailableLocales(localeCount);

    for (i = 0; i < localeCount; i += 1)
    {
        Collator *c = NULL;

        status = U_ZERO_ERROR;
        c = Collator::createInstance(locales[i], status);

        if (c == NULL || U_FAILURE(status))
        {
            UnicodeString msg, localeName;

            msg += "Could not create collator for locale ";
            msg += locales[i].getName();

            errln(msg);
        }

        delete c;
    }
}






void CollationRegressionTest::Test4139572()
{
    
    
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    Locale l("es", "es");
    Collator *col = NULL;

    col = Collator::createInstance(l, status);

    if (col == NULL || U_FAILURE(status))
    {
        errln("Failed to create a collator for es_es locale.");
        delete col;
        return;
    }

    CollationKey key;

    
    col->getCollationKey("Nombre De Objeto", key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("Error creating CollationKey for \"Nombre De Ojbeto\"");
    }

    delete col;
}



class My4146160Collator : public RuleBasedCollator
{
public:
    My4146160Collator(RuleBasedCollator &rbc, UErrorCode &status);
    ~My4146160Collator();

    CollationElementIterator *createCollationElementIterator(const UnicodeString &text) const;

    CollationElementIterator *createCollationElementIterator(const CharacterIterator &text) const;

    static int32_t count;
};

int32_t My4146160Collator::count = 0;

My4146160Collator::My4146160Collator(RuleBasedCollator &rbc, UErrorCode &status)
  : RuleBasedCollator(rbc.getRules(), status)
{
}

My4146160Collator::~My4146160Collator()
{
}

CollationElementIterator *My4146160Collator::createCollationElementIterator(const UnicodeString &text) const
{
    count += 1;
    return RuleBasedCollator::createCollationElementIterator(text);
}

CollationElementIterator *My4146160Collator::createCollationElementIterator(const CharacterIterator &text) const
{
    count += 1;
    return RuleBasedCollator::createCollationElementIterator(text);
}





void CollationRegressionTest::Test4146160()
{
#if 0
    
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    CollationKey key;

    My4146160Collator::count = 0;
    My4146160Collator *mc = NULL;

    mc = new My4146160Collator(*en_us, status);

    if (mc == NULL || U_FAILURE(status))
    {
        errln("Failed to create a My4146160Collator.");
        delete mc;
        return;
    }

    mc->getCollationKey("1", key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("Failure to get a CollationKey from a My4146160Collator.");
        delete mc;
        return;
    }

    if (My4146160Collator::count < 1)
    {
        errln("My4146160Collator::createCollationElementIterator not called for getCollationKey");
    }

    My4146160Collator::count = 0;
    mc->compare("1", "2");

    if (My4146160Collator::count < 1)
    {
        errln("My4146160Collator::createtCollationElementIterator not called for compare");
    }

    delete mc;
#endif
}

void CollationRegressionTest::Test4179216() {
    
    
    
    IcuTestErrorCode errorCode(*this, "Test4179216");
    RuleBasedCollator coll(en_us->getRules() + " & C < ch , cH , Ch , CH < cat < crunchy", errorCode);
    UnicodeString testText = "church church catcatcher runcrunchynchy";
    CollationElementIterator *iter = coll.createCollationElementIterator(testText);

    
    iter->setOffset(4, errorCode);
    int32_t elt4 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->reset();
    int32_t elt0 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(5, errorCode);
    int32_t elt5 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    
    if (elt4 != elt0 || elt5 != elt0) {
        errln("The collation elements at positions 0 (0x%04x), "
                "4 (0x%04x), and 5 (0x%04x) don't match.",
                elt0, elt4, elt5);
    }

    
    iter->setOffset(14, errorCode);
    int32_t elt14 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(15, errorCode);
    int32_t elt15 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(16, errorCode);
    int32_t elt16 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(17, errorCode);
    int32_t elt17 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(18, errorCode);
    int32_t elt18 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    iter->setOffset(19, errorCode);
    int32_t elt19 = CollationElementIterator::primaryOrder(iter->next(errorCode));

    
    if (elt14 != elt15 || elt14 != elt16 || elt14 != elt17
            || elt14 != elt18 || elt14 != elt19) {
        errln("\"cat\" elements don't match: elt14 = 0x%04x, "
                "elt15 = 0x%04x, elt16 = 0x%04x, elt17 = 0x%04x, "
                "elt18 = 0x%04x, elt19 = 0x%04x",
                elt14, elt15, elt16, elt17, elt18, elt19);
    }

    
    
    
    iter->reset();

    int32_t elt = iter->next(errorCode);
    int32_t count = 0;
    while (elt != CollationElementIterator::NULLORDER) {
        ++count;
        elt = iter->next(errorCode);
    }

    LocalArray<UnicodeString> nextElements(new UnicodeString[count]);
    LocalArray<UnicodeString> setOffsetElements(new UnicodeString[count]);
    int32_t lastPos = 0;

    iter->reset();
    elt = iter->next(errorCode);
    count = 0;
    while (elt != CollationElementIterator::NULLORDER) {
        nextElements[count++] = testText.tempSubStringBetween(lastPos, iter->getOffset());
        lastPos = iter->getOffset();
        elt = iter->next(errorCode);
    }
    int32_t nextElementsLength = count;
    count = 0;
    for (int32_t i = 0; i < testText.length(); ) {
        iter->setOffset(i, errorCode);
        lastPos = iter->getOffset();
        elt = iter->next(errorCode);
        setOffsetElements[count++] = testText.tempSubStringBetween(lastPos, iter->getOffset());
        i = iter->getOffset();
    }
    for (int32_t i = 0; i < nextElementsLength; i++) {
        if (nextElements[i] == setOffsetElements[i]) {
            logln(nextElements[i]);
        } else {
            errln(UnicodeString("Error: next() yielded ") + nextElements[i] +
                ", but setOffset() yielded " + setOffsetElements[i]);
        }
    }
    delete iter;
}




static int32_t calcKeyIncremental(UCollator *coll, const UChar* text, int32_t len, uint8_t *keyBuf, int32_t , UErrorCode& status) {
    UCharIterator uiter;
    uint32_t state[2] = { 0, 0 };
    int32_t keyLen;
    int32_t count = 8;

    uiter_setString(&uiter, text, len);
    keyLen = 0;
    while (TRUE) {
        int32_t keyPartLen = ucol_nextSortKeyPart(coll, &uiter, state, &keyBuf[keyLen], count, &status);
        if (U_FAILURE(status)) {
            return -1;
        }
        if (keyPartLen == 0) {
            break;
        }
        keyLen += keyPartLen;
    }
    return keyLen;
}

void CollationRegressionTest::TestT7189() {
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll;
    uint32_t i;

    static const UChar text1[][CollationRegressionTest::MAX_TOKEN_LEN] = {
    
        { 0x41, 0x63, 0x68, 0x74, 0x65, 0x72, 0x20, 0x44, 0x65, 0x20, 0x48, 0x6F, 0x76, 0x65, 0x6E, 0x00 },
        
        { 0x41, 0x42, 0x43, 0x00 },
        
        { 0x48, 0x45, 0x4C, 0x4C, 0x4F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00 }
    };

    static const UChar text2[][CollationRegressionTest::MAX_TOKEN_LEN] = {
    
        { 0x41, 0x63, 0x68, 0x74, 0x65, 0x72, 0x20, 0x64, 0x65, 0x20, 0x48, 0x6F, 0x76, 0x65, 0x6E, 0x00 },
        
        { 0x61, 0x62, 0x63, 0x00 },
        
        { 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00 }
    };

    
    coll = ucol_openFromShortString("EO_S1", FALSE, NULL, &status);
    if (U_FAILURE(status)) {
        errln("Failed to create a collator for short string EO_S1");
        return;
    }

    for (i = 0; i < sizeof(text1) / (CollationRegressionTest::MAX_TOKEN_LEN * sizeof(UChar)); i++) {
        uint8_t key1[100], key2[100];
        int32_t len1, len2;

        len1 = calcKeyIncremental(coll, text1[i], -1, key1, sizeof(key1), status);
        if (U_FAILURE(status)) {
            errln(UnicodeString("Failed to get a partial collation key for ") + text1[i]);
            break;
        }
        len2 = calcKeyIncremental(coll, text2[i], -1, key2, sizeof(key2), status);
        if (U_FAILURE(status)) {
            errln(UnicodeString("Failed to get a partial collation key for ") + text2[i]);
            break;
        }

        if (len1 == len2 && uprv_memcmp(key1, key2, len1) == 0) {
            errln(UnicodeString("Failed: Identical key\n") + "    text1: " + text1[i] + "\n" + "    text2: " + text2[i] + "\n" + "    key  : " + TestUtility::hex(key1, len1));
        } else {
            logln(UnicodeString("Keys produced -\n") + "    text1: " + text1[i] + "\n" + "    key1 : " + TestUtility::hex(key1, len1) + "\n" + "    text2: " + text2[i] + "\n" + "    key2 : "
                    + TestUtility::hex(key2, len2));
        }
    }
    ucol_close(coll);
}

void CollationRegressionTest::TestCaseFirstCompression() {
    RuleBasedCollator *col = (RuleBasedCollator *) en_us->clone();
    UErrorCode status = U_ZERO_ERROR;

    
    caseFirstCompressionSub(col, "default");

    
    col->setAttribute(UCOL_CASE_FIRST, UCOL_UPPER_FIRST, status);
    if (U_FAILURE(status)) {
        errln("Failed to set UCOL_UPPER_FIRST");
        return;
    }
    caseFirstCompressionSub(col, "upper first");

    
    col->setAttribute(UCOL_CASE_FIRST, UCOL_LOWER_FIRST, status);
    if (U_FAILURE(status)) {
        errln("Failed to set UCOL_LOWER_FIRST");
        return;
    }
    caseFirstCompressionSub(col, "lower first");

    delete col;
}

void CollationRegressionTest::caseFirstCompressionSub(Collator *col, UnicodeString opt) {
    const int32_t maxLength = 50;

    UChar str1[maxLength];
    UChar str2[maxLength];

    CollationKey key1, key2;

    for (int32_t len = 1; len <= maxLength; len++) {
        int32_t i = 0;
        for (; i < len - 1; i++) {
            str1[i] = str2[i] = (UChar)0x61; 
        }
        str1[i] = (UChar)0x41; 
        str2[i] = (UChar)0x61; 

        UErrorCode status = U_ZERO_ERROR;
        col->getCollationKey(str1, len, key1, status);
        col->getCollationKey(str2, len, key2, status);

        UCollationResult cmpKey = key1.compareTo(key2, status);
        UCollationResult cmpCol = col->compare(str1, len, str2, len, status);

        if (U_FAILURE(status)) {
            errln("Error in caseFirstCompressionSub");
        } else if (cmpKey != cmpCol) {
            errln((UnicodeString)"Inconsistent comparison(" + opt
                + "): str1=" + UnicodeString(str1, len) + ", str2=" + UnicodeString(str2, len)
                + ", cmpKey=" + cmpKey + ", cmpCol=" + cmpCol);
        }
    }
}

void CollationRegressionTest::TestTrailingComment() {
    
    
    IcuTestErrorCode errorCode(*this, "TestTrailingComment");
    RuleBasedCollator coll(UNICODE_STRING_SIMPLE("&c<b#comment1\n<a#comment2"), errorCode);
    UnicodeString a((UChar)0x61), b((UChar)0x62), c((UChar)0x63);
    assertTrue("c<b", coll.compare(c, b) < 0);
    assertTrue("b<a", coll.compare(b, a) < 0);
}

void CollationRegressionTest::TestBeforeWithTooStrongAfter() {
    
    
    IcuTestErrorCode errorCode(*this, "TestBeforeWithTooStrongAfter");
    RuleBasedCollator before2(UNICODE_STRING_SIMPLE("&[before 2]x<<q<p"), errorCode);
    if(errorCode.isSuccess()) {
        errln("should forbid before-2-reset followed by primary relation");
    } else {
        errorCode.reset();
    }
    RuleBasedCollator before3(UNICODE_STRING_SIMPLE("&[before 3]x<<<q<<s<p"), errorCode);
    if(errorCode.isSuccess()) {
        errln("should forbid before-3-reset followed by primary or secondary relation");
    } else {
        errorCode.reset();
    }
}

void CollationRegressionTest::compareArray(Collator &c,
                                           const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN],
                                           int32_t testCount)
{
    int32_t i;
    Collator::EComparisonResult expectedResult = Collator::EQUAL;

    for (i = 0; i < testCount; i += 3)
    {
        UnicodeString source(tests[i]);
        UnicodeString comparison(tests[i + 1]);
        UnicodeString target(tests[i + 2]);

        if (comparison == "<")
        {
            expectedResult = Collator::LESS;
        }
        else if (comparison == ">")
        {
            expectedResult = Collator::GREATER;
        }
        else if (comparison == "=")
        {
            expectedResult = Collator::EQUAL;
        }
        else
        {
            UnicodeString bogus1("Bogus comparison string \"");
            UnicodeString bogus2("\"");
            errln(bogus1 + comparison + bogus2);
        }

        Collator::EComparisonResult compareResult = c.compare(source, target);

        CollationKey sourceKey, targetKey;
        UErrorCode status = U_ZERO_ERROR;

        c.getCollationKey(source, sourceKey, status);

        if (U_FAILURE(status))
        {
            errln("Couldn't get collationKey for source");
            continue;
        }

        c.getCollationKey(target, targetKey, status);

        if (U_FAILURE(status))
        {
            errln("Couldn't get collationKey for target");
            continue;
        }

        Collator::EComparisonResult keyResult = sourceKey.compareTo(targetKey);

        reportCResult( source, target, sourceKey, targetKey, compareResult, keyResult, compareResult, expectedResult );

    }
}

void CollationRegressionTest::assertEqual(CollationElementIterator &i1, CollationElementIterator &i2)
{
    int32_t c1, c2, count = 0;
    UErrorCode status = U_ZERO_ERROR;

    do
    {
        c1 = i1.next(status);
        c2 = i2.next(status);

        if (c1 != c2)
        {
            UnicodeString msg, msg1("    ");

            msg += msg1 + count;
            msg += ": strength(0x";
            appendHex(c1, 8, msg);
            msg += ") != strength(0x";
            appendHex(c2, 8, msg);
            msg += ")";

            errln(msg);
            break;
        }

        count += 1;
    }
    while (c1 != CollationElementIterator::NULLORDER);
}

void CollationRegressionTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* )
{
    if (exec)
    {
        logln("Collation Regression Tests: ");
    }

    if(en_us == NULL) {
        dataerrln("Class collator not instantiated");
        name = "";
        return;
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(Test4048446);
    TESTCASE_AUTO(Test4051866);
    TESTCASE_AUTO(Test4053636);
    TESTCASE_AUTO(Test4054238);
    TESTCASE_AUTO(Test4054734);
    TESTCASE_AUTO(Test4054736);
    TESTCASE_AUTO(Test4058613);
    TESTCASE_AUTO(Test4059820);
    TESTCASE_AUTO(Test4060154);
    TESTCASE_AUTO(Test4062418);
    TESTCASE_AUTO(Test4065540);
    TESTCASE_AUTO(Test4066189);
    TESTCASE_AUTO(Test4066696);
    TESTCASE_AUTO(Test4076676);
    TESTCASE_AUTO(Test4078588);
    TESTCASE_AUTO(Test4079231);
    TESTCASE_AUTO(Test4081866);
    TESTCASE_AUTO(Test4087241);
    TESTCASE_AUTO(Test4087243);
    TESTCASE_AUTO(Test4092260);
    TESTCASE_AUTO(Test4095316);
    TESTCASE_AUTO(Test4101940);
    TESTCASE_AUTO(Test4103436);
    TESTCASE_AUTO(Test4114076);
    TESTCASE_AUTO(Test4114077);
    TESTCASE_AUTO(Test4124632);
    TESTCASE_AUTO(Test4132736);
    TESTCASE_AUTO(Test4133509);
    TESTCASE_AUTO(Test4139572);
    TESTCASE_AUTO(Test4141640);
    TESTCASE_AUTO(Test4146160);
    TESTCASE_AUTO(Test4179216);
    TESTCASE_AUTO(TestT7189);
    TESTCASE_AUTO(TestCaseFirstCompression);
    TESTCASE_AUTO(TestTrailingComment);
    TESTCASE_AUTO(TestBeforeWithTooStrongAfter);
    TESTCASE_AUTO_END;
}

#endif 
