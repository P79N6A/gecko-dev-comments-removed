















function testcase() {

        try {
            eval("function _13_1_26_fun(param1, param2, param1) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
