






#include "jsapi-tests/tests.h"

BEGIN_TEST(testIntlAvailableLocales)
{
    
    JS::Rooted<JS::Value> haveIntl(cx);
    EVAL("typeof Intl !== 'undefined'", &haveIntl);
    if (!haveIntl.toBoolean())
        return true;

    
    
    CHECK(JS_SetDefaultLocale(rt, "de-ZA-abcde-x-private"));

    EXEC("if (Intl.Collator().resolvedOptions().locale !== 'de-ZA-abcde-x-private') \n"
         "    throw 'unexpected default locale';");
    EXEC("var used = Intl.Collator('de-ZA-abcde').resolvedOptions().locale; \n"
         "if (used !== 'de-ZA-abcde') \n"
         "    throw 'bad locale when using truncated default: ' + used;");
    EXEC("if (Intl.Collator('de-ZA').resolvedOptions().locale !== 'de-ZA') \n"
         "    throw 'bad locale when using more-truncated default';");
    EXEC("if (Intl.Collator('de-ZA-ghijk').resolvedOptions().locale !== 'de-ZA') \n"
         "    throw 'unexpected default locale';");

    EXEC("if (Intl.Collator('de-ZA-abcde-x-private', { localeMatcher: 'lookup' }).resolvedOptions().locale !== \n"
         "    'de-ZA-abcde-x-private') \n"
         "{ \n"
         "    throw 'unexpected default locale with lookup matcher'; \n"
         "}");
    EXEC("if (Intl.Collator('de-ZA-abcde').resolvedOptions().locale !== 'de-ZA-abcde') \n"
         "    throw 'bad locale when using truncated default';");
    EXEC("if (Intl.Collator('de-ZA').resolvedOptions().locale !== 'de-ZA') \n"
         "    throw 'bad locale when using more-truncated default';");
    EXEC("if (Intl.Collator('de').resolvedOptions().locale !== 'de') \n"
         "    throw 'bad locale when using most-truncated default';");

    CHECK(JS_SetDefaultLocale(rt, "en-US-u-co-phonebk"));
    EXEC("if (Intl.Collator().resolvedOptions().locale !== 'en-US') \n"
         "    throw 'unexpected default locale where proposed default included a Unicode extension';");

    CHECK(JS_SetDefaultLocale(rt, "this is not a language tag at all, yo"));

    EXEC("if (Intl.Collator().resolvedOptions().locale !== 'en-GB') \n"
         "    throw 'unexpected last-ditch locale';");
    EXEC("if (Intl.Collator('en-GB').resolvedOptions().locale !== 'en-GB') \n"
         "    throw 'unexpected used locale when specified, with last-ditch locale as default';");

    JS_ResetDefaultLocale(rt);
    return true;
}
END_TEST(testIntlAvailableLocales)
