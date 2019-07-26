











function testcase() {
        "use strict";
        var arrObj = [1,2,3];

        try {
            eval("delete arrObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
