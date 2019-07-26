











function testcase() {
        "use strict";
        var errObj = new Error();

        try {
            eval("delete errObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
