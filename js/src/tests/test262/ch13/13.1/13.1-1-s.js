















function testcase() {
        "use strict";

        try {
            eval("function _13_1_1_fun(eval) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
