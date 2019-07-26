











function testcase() {
    "use strict";
    var a = new RegExp();
    try {
        var b = delete RegExp.length;
        return false;
    } catch (e) {
        return e instanceof TypeError;
    }
}
runTestCase(testcase);
