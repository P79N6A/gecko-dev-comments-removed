













function testcase() {
        "use strict";

        try {
            eval("var private = 1;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
}
runTestCase(testcase);