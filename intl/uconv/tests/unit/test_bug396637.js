

load('CharsetConversionTests.js');

const inString = "A";
const expectedString = "";
const charset = "UTF-16BE";

function run_test() {
    checkDecode(CreateScriptableConverter(), charset, inString, expectedString);
}
