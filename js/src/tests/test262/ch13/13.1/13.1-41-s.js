











function testcase() {
        var _13_1_41_s = {};
        try {
            eval("'use strict'; _13_1_41_s.x = function arguments() {};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
