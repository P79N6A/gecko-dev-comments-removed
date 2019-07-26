







$INCLUDE("testIntl.js");

var invalidLanguageTags = [
    "", 
    "i", 
    "x", 
    "u", 
    "419", 
    "u-nu-latn-cu-bob", 
    "hans-cmn-cn", 
                   
    "cmn-hans-cn-u-u", 
    "cmn-hans-cn-t-u-ca-u", 
    "de-gregory-gregory", 
    "*", 
    "de-*", 
    "中文", 
    "en-ß", 
    "ıd" 
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

