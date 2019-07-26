











function testcase() {
        "use strict";
        try {
            eval("with ({}) { throw new Error();}");

            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
