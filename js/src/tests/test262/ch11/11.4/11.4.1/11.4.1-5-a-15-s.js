











function testcase() {
        "use strict";
        var regObj = new RegExp();

        try {
            eval("delete regObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
