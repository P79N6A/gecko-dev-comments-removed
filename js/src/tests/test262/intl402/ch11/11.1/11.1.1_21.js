







$INCLUDE("testIntl.js");

testOption(Intl.NumberFormat, "currencyDisplay", "string", ["code", "symbol", "name"],
    "symbol", {extra: {any: {style: "currency", currency: "XDR"}}});
testOption(Intl.NumberFormat, "currencyDisplay", "string", ["code", "symbol", "name"],
    undefined, {noReturn: true});

