















function testcase() {

        try {
            eval("var _13_1_22_fun = function (arguments) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
