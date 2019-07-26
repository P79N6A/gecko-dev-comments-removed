











function testcase() {
        "use strict";
        var dateObj = new Date();

        try {
            eval("delete dateObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
