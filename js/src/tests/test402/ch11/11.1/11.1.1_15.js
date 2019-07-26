







$INCLUDE("testIntl.js");

testOption(Intl.NumberFormat, "style", "string", ["decimal", "percent", "currency"], "decimal",
        {extra: {"currency": {currency: "CNY"}}});

