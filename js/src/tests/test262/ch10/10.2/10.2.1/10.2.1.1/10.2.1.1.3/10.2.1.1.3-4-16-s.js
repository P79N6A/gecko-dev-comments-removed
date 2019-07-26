











function testcase() {
        "use strict";

        try {
            NaN = 12;
            return false;
        } catch (e) {
            return e instanceof TypeError;
        }
    }
runTestCase(testcase);
