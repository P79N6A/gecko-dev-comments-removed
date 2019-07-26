















function testcase() {
        "use strict";

        try {
            eval("var _13_1_4_fun = function (arguments) { };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
