














function testcase() {

        function _13_0_11_fun() {
            "use strict";
            function _13_0_11_inner() {
                eval("eval = 42;");
            }
            _13_0_11_inner();
        };
        try {
            _13_0_11_fun();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
