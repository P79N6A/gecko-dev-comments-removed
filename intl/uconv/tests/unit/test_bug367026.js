









load('CharsetConversionTests.js');

const inASCII = "Hello World";
const inHanzi = "\u4E00";
const inMixed = "Hello \u4E00 World";
    
const expectedASCII = "Hello World";
const expectedHanzi = "一";
const expectedMixed = "Hello 一 World";

const charset = "HZ-GB-2312";
    
function run_test() {
    var converter = CreateScriptableConverter();

    checkEncode(converter, charset, inASCII, expectedASCII);
    checkEncode(converter, charset, inMixed, expectedMixed);
    checkEncode(converter, charset, inHanzi, expectedHanzi);
}
