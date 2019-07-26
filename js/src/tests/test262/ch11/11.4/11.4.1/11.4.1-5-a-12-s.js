











function testcase() {
        "use strict";
        var boolObj = new Boolean(false);

        try {
            eval("delete boolObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
