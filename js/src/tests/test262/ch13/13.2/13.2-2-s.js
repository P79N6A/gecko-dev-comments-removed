











function testcase() {
        "use strict";
        try {
            var foo = function () {
            }
            foo.caller = 20;
            return false;
        } catch (ex) {
            return ex instanceof TypeError;
        }
    }
runTestCase(testcase);
