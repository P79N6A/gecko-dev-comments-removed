











function testcase() {
        "use strict";
        try {
            eval("function eval() { };")
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
