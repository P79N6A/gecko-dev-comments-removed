











function testcase() {
        var _13_1_42_s = {};
        try {
            eval("_13_1_42_s.x = function arguments() {'use strict';};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
