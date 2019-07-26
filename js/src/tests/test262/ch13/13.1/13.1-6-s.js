















function testcase() {
        "use strict";

        try {
            eval("function _13_1_6_fun(param1, param2, param1) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
