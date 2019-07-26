











function testcase() {
        "use strict";
        var strObj = new String("abc");

        try {
            eval("delete strObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
