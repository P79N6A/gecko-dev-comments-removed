











function testcase() {
        "use strict";

        try {
            throw new Error("...");
            return false;
        } catch (Arguments) {
            return Arguments instanceof Error;
        }
    }
runTestCase(testcase);
