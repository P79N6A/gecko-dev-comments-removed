











function testcase() {
        "use strict";

        try {
            eval("delete Number;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
