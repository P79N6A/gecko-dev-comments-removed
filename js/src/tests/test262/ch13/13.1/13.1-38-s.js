











function testcase() {
        var _13_1_38_s = {};
        try {
            eval("_13_1_38_s.x = function eval() {'use strict'; };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
