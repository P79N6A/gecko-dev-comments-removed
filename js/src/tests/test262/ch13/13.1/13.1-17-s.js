















function testcase() {

        try {
            eval("'use strict'; var _13_1_17_fun = function (eval) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
