















function testcase() {
        "use strict";

        try {
            eval("function _13_1_7_fun(param, param, param) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
