











function testcase() {
        "use strict";

        try {
            throw new Error("...");
            return false;
        } catch (ARGUMENTS) {
            return ARGUMENTS instanceof Error;
        }
    }
runTestCase(testcase);
