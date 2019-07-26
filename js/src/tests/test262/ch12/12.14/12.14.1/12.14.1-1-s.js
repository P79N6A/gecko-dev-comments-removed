











function testcase() {
        "use strict";

        try {
            eval("\
                   try {} catch (eval) { }\
            ");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
