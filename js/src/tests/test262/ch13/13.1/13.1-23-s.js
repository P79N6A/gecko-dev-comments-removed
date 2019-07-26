















function testcase() {

        try {
            eval("'use strict'; function _13_1_23_fun(param, param) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
