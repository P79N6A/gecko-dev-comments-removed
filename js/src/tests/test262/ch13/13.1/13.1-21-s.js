















function testcase() {

        try {
            eval("'use strict'; var _13_1_21_fun = function (arguments) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
