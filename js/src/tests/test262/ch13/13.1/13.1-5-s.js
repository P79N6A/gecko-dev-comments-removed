















function testcase() {
        "use strict";

        try {
            eval("function _13_1_5_fun(param, param) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
