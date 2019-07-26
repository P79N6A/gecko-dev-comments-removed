











function testcase() {
        "use strict";
        var _13_1_14_s = {};

        try {
            eval("_13_1_14_s.x = function arguments() {};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
