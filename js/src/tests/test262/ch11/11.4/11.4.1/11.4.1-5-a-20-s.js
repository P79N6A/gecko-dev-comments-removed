











function testcase() {
        "use strict";

        try {
            eval("delete Array;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
