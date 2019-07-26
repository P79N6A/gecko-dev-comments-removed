








$INCLUDE("testIntl.js");

var actual = new Intl.NumberFormat().resolvedOptions();

var actual2 = new Intl.NumberFormat().resolvedOptions();
if (actual2 === actual) {
    $ERROR("resolvedOptions returned the same object twice.");
}


mustHaveProperty(actual, "locale", isCanonicalizedStructurallyValidLanguageTag);
mustHaveProperty(actual, "numberingSystem", isValidNumberingSystem);
mustHaveProperty(actual, "style", ["decimal"]);
mustNotHaveProperty(actual, "currency");
mustNotHaveProperty(actual, "currencyDisplay");
mustHaveProperty(actual, "minimumIntegerDigits", [1]);
mustHaveProperty(actual, "minimumFractionDigits", [0]);
mustHaveProperty(actual, "maximumFractionDigits", [3]);
mustNotHaveProperty(actual, "minimumSignificantDigits");
mustNotHaveProperty(actual, "maximumSignificantDigits");
mustHaveProperty(actual, "useGrouping", [true]);

