











function testcase() {
        "use strict";
        var _11_4_1_5 = 5;

        try {
            eval("delete _11_4_1_5;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
