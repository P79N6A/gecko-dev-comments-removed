











function testcase() {
        "use strict";

        try {
            eval("delete RegExp;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
