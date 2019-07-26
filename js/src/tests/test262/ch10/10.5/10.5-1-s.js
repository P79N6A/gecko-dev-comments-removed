











function testcase() {
        "use strict";
        try {
            (function fun() {
                eval("arguments = 10");
            })(30);
            return false;
        } catch (e) {
            return (e instanceof SyntaxError);
        }
    }
runTestCase(testcase);
