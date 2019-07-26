











function testcase() {
        "use strict";

        try {
            eval("delete Date;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
