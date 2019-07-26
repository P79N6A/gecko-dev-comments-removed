











function testcase() {
        var _13_1_37_s = {};
        try {
            eval("'use strict'; _13_1_37_s.x = function eval() {};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
