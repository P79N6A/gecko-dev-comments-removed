







$INCLUDE("testIntl.js");

var invalidLanguageTags = [
    "de_DE",
    "DE_de",
    "cmn_Hans",
    "cmn-hans_cn",
    "es_419",
    "es-419-u-nu-latn-cu_bob",
    "i_klingon",
    "cmn-hans-cn-t-ca-u-ca-x_t-u",
    "enochian_enochian",
    "de-gregory_u-ca-gregory"
];

testWithIntlConstructors(function (Constructor) {
    invalidLanguageTags.forEach(function (tag) {
        var error;
        try {
            
            var obj = new Constructor([tag]);
        } catch (e) {
            error = e;
        }
        if (error === undefined) {
            $ERROR("Invalid language tag " + tag + " was not rejected.");
        } else if (error.name !== "RangeError") {
            $ERROR("Invalid language tag " + tag + " was rejected with wrong error " + error.name + ".");
        }
    });
    return true;
});

