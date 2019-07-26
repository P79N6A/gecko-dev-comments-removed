











function testcase() {
        "use strict";

        try {
            eval("delete String;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
