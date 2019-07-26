











function testcase() {
        "use strict";
        try {
            eval("_11_13_2_8 >>>= 1;");
            return false;
        } catch (e) {
            return e instanceof ReferenceError;
        }
    }
runTestCase(testcase);
