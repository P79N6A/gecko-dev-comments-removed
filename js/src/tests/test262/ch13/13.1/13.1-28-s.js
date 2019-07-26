















function testcase() {
        

        try {
            eval("function _13_1_28_fun(param, param, param) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
