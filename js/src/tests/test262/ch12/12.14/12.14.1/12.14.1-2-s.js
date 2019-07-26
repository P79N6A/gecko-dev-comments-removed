











function testcase() {
        "use strict";

        try {
            eval("\
                   try {} catch (arguments) { }\
            ");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
