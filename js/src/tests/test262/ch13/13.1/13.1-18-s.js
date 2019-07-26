















function testcase() {

        try {
            eval("var _13_1_18_fun = function (eval) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
