








$INCLUDE("testIntl.js");

var actual = new Intl.Collator().resolvedOptions();

var actual2 = new Intl.Collator().resolvedOptions();
if (actual2 === actual) {
    $ERROR("resolvedOptions returned the same object twice.");
}


var collations = [
    "default", 
    "big5han", 
    "dict",
    "direct",
    "ducet",
    "gb2312",
    "phonebk",
    "phonetic",
    "pinyin",
    "reformed",
    
    "searchjl",
    
    "stroke",
    "trad",
    "unihan"
];


mustHaveProperty(actual, "locale", isCanonicalizedStructurallyValidLanguageTag);
mustHaveProperty(actual, "usage", ["sort"]);
mustHaveProperty(actual, "sensitivity", ["variant"]);
mustHaveProperty(actual, "ignorePunctuation", [false]);
mustHaveProperty(actual, "collation", collations);
mayHaveProperty(actual, "numeric", [true, false]);
mayHaveProperty(actual, "caseFirst", ["upper", "lower", "false"]);

