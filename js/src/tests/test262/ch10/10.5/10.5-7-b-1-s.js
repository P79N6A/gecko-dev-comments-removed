











function testcase() {
        "use strict";

        try {
            eval("(function _10_5_7_b_1_fun() { arguments = 10;} ());");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
