











function testcase() {
        "use strict";

        try {
            eval("var obj = {}; obj['get'] = function (a) { with(a){} };  ");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
