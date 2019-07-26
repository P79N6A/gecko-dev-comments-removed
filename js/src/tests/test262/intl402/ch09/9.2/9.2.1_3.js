








$INCLUDE("testIntl.js");

var validAndInvalidLanguageTags = [
    "de", 
    "de-DE", 
    "DE-de", 
    "cmn", 
    "cmn-Hans", 
    "CMN-hANS", 
    "cmn-hans-cn", 
    "es-419", 
    "es-419-u-nu-latn-cu-bob", 
    "i-klingon", 
    "cmn-hans-cn-t-ca-u-ca-x-t-u", 
    "enochian-enochian", 
    "de-gregory-u-ca-gregory", 
    "de_DE",
    "DE_de",
    "cmn_Hans",
    "cmn-hans_cn",
    "es_419",
    "es-419-u-nu-latn-cu_bob",
    "i_klingon",
    "cmn-hans-cn-t-ca-u-ca-x_t-u",
    "enochian_enochian",
    "de-gregory_u-ca-gregory",
    "i", 
    "x", 
    "u", 
    "419", 
    "u-nu-latn-cu-bob", 
    "hans-cmn-cn", 
                   
    "cmn-hans-cn-u-u", 
    "cmn-hans-cn-t-u-ca-u", 
    "de-gregory-gregory" 
];

testWithIntlConstructors(function (Constructor) {
    validAndInvalidLanguageTags.forEach(function (locale) {
        var obj1, obj2, locale1, locale2, error1, error2;
        try {
            obj1 = new Constructor(locale);
            locale1 = obj1.resolvedOptions().locale;
        } catch (e) {
            error1 = e;
        }
        try {
            obj2 = new Constructor([locale]);
            locale2 = obj2.resolvedOptions().locale;
        } catch (e) {
            error2 = e;
        }

        if ((error1 === undefined) !== (error2 === undefined)) {
            if (error1 === undefined) {
                $ERROR("Single locale string " + locale +
                    " was accepted, but locale list containing that string wasn't.");
            } else {
                $ERROR("Single locale string " + locale +
                    " was rejected, but locale list containing that string wasn't.");
            }
        } else if (error1 === undefined) {
             if (locale1 !== locale2) {
                $ERROR("Single locale string " + locale + " results in " + locale1 +
                    ", but locale list [" + locale + "] results in " + locale2 + ".");
             }
        } else {
            if (error1.name !== error2.name) {
                $ERROR("Single locale string " + locale + " results in error " + error1.name +
                    ", but locale list [" + locale + "] results in error " + error2.name + ".");
             }
        }
    });
    
    return true;
});

