



#include "jsapi-tests/tests.h"

#ifdef JS_HAS_SYMBOLS

BEGIN_TEST(testSymbol_New)
{
    using namespace JS;

    RootedString desc(cx, nullptr);
    RootedSymbol sym1(cx);
    CHECK(sym1 = NewSymbol(cx, desc));
    CHECK_NULL(GetSymbolDescription(sym1));
    RootedValue v(cx, SymbolValue(sym1));
    CHECK_EQUAL(JS_TypeOfValue(cx, v), JSTYPE_SYMBOL);

    RootedSymbol sym2(cx);
    CHECK(sym2 = NewSymbol(cx, desc));
    CHECK(sym1 != sym2);

    CHECK(desc = JS_NewStringCopyZ(cx, "ponies"));
    CHECK(sym2 = NewSymbol(cx, desc));
    CHECK_SAME(StringValue(GetSymbolDescription(sym2)), StringValue(desc));

    return true;
}
END_TEST(testSymbol_New)

BEGIN_TEST(testSymbol_GetSymbolFor)
{
    using namespace JS;

    RootedString desc(cx, JS_NewStringCopyZ(cx, "ponies"));
    CHECK(desc);
    RootedSymbol sym1(cx);
    CHECK(sym1 = GetSymbolFor(cx, desc));
    CHECK_SAME(StringValue(GetSymbolDescription(sym1)), StringValue(desc));

    
    
    RootedSymbol sym2(cx);
    CHECK(sym2 = GetSymbolFor(cx, desc));
    CHECK_EQUAL(sym1, sym2);

    
    CHECK(desc = JS_NewStringCopyZ(cx, "ponies"));
    CHECK(sym2 = GetSymbolFor(cx, desc));
    CHECK_EQUAL(sym1, sym2);

    
    CHECK(sym2 = NewSymbol(cx, desc));
    CHECK(sym2 != sym1);

    return true;
}
END_TEST(testSymbol_GetSymbolFor)

BEGIN_TEST(testSymbol_GetWellKnownSymbol)
{
    using namespace JS;

    Rooted<Symbol*> sym1(cx);
    CHECK(sym1 = GetWellKnownSymbol(cx, SymbolCode::iterator));
    RootedValue v(cx);
    EVAL("Symbol.iterator", &v);
    CHECK_SAME(v, SymbolValue(sym1));

    
    RootedString desc(cx);
    CHECK(desc = JS_NewStringCopyZ(cx, "Symbol.iterator"));
    CHECK_SAME(StringValue(GetSymbolDescription(sym1)), StringValue(desc));

    
    Rooted<Symbol*> sym2(cx);
    CHECK(sym2 = GetSymbolFor(cx, desc));
    CHECK(sym2 != sym1);

    return true;
}
END_TEST(testSymbol_GetWellKnownSymbol)

#endif
