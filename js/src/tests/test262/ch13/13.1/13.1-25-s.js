















function testcase() {

        try {
            eval("'use strict'; function _13_1_25_fun(param1, param2, param1) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
