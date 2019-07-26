











function testcase() {
        "use strict";

        try {
            eval("function arguments() { };")
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
