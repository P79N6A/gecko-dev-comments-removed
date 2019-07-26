











function testcase() {
        "use strict";
        var obj = new Object();

        try {
            eval("delete obj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
