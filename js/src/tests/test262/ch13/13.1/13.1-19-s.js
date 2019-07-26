















function testcase() {

        try {
            eval("'use strict';function _13_1_19_fun(arguments) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
