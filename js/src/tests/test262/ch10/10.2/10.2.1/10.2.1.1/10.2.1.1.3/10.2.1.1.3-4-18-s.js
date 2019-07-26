











function testcase() {
        "use strict";
        try {
            undefined = 12;
            return false;
        } catch (e) {
            return e instanceof TypeError;
        }
    }
runTestCase(testcase);
