















function testcase() {

        try {
            eval("var _13_1_34_fun = function (param, param, param) { 'use strict'; };")
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
