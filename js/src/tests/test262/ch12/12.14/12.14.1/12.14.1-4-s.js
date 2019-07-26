











function testcase() {
        "use strict";

        try {
            throw new Error("...");
            return false;
        } catch (EVAL) {
            return EVAL instanceof Error;
        }
    }
runTestCase(testcase);
