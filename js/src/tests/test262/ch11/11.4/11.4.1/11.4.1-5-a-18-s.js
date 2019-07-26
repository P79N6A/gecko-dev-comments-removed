











function testcase() {
        "use strict";

        try {
            eval("delete Object;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
