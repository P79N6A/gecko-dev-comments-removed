











function testcase() {
        "use strict";
        try {
            var foo = function () {
            }
            foo.arguments = 20;
            return false;
        } catch (ex) {
            return ex instanceof TypeError;
        }
    }
runTestCase(testcase);
