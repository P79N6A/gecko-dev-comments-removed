











function testcase() {
        "use strict";
        try {
            eval("_8_7_2_1 = 11;");
            return false;
        } catch (e) {
            return e instanceof ReferenceError;
        }
    }
runTestCase(testcase);
