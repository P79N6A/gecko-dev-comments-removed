











function testcase() {
        "use strict";

        try {
            eval("delete Boolean;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
