







$INCLUDE("testIntl.js");

var validLanguageTags = [
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
    "aa-a-foo-x-a-foo-bar", 
    "x-en-US-12345", 
    "x-12345-12345-en-US",
    "x-en-US-12345-12345",
    "x-en-u-foo",
    "x-en-u-foo-u-bar"
];

testWithIntlConstructors(function (Constructor) {
    validLanguageTags.forEach(function (tag) {
        
        var obj = new Constructor([tag]);
    });
    return true;
});

