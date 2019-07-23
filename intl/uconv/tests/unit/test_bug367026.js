









const inASCII = "Hello World";
const inHanzi = "\u4E00";
const inMixed = "Hello \u4E00 World";
    
const expectedASCII = "Hello World";
const expectedHanzi = "一";
const expectedMixed = "Hello 一 World";

const charset = "HZ-GB-2312";
    
function run_test() {
    var ScriptableUnicodeConverter =
	Components.Constructor("@mozilla.org/intl/scriptableunicodeconverter",
			       "nsIScriptableUnicodeConverter");

    var converter = new ScriptableUnicodeConverter();
    converter.charset = charset;

    var outASCII = converter.ConvertFromUnicode(inASCII) + converter.Finish();
    do_check_eq(outASCII, expectedASCII);

    var outMixed = converter.ConvertFromUnicode(inMixed) + converter.Finish();
    do_check_eq(outMixed, expectedMixed);

    var outHanzi = converter.ConvertFromUnicode(inHanzi) + converter.Finish();
    do_check_eq(outHanzi, expectedHanzi);
}
