











function testcase() {
        "use strict";

        function _10_5_7_b_2_fun() {
            arguments[7] = 12;
            return arguments[7] === 12;
        };

        return _10_5_7_b_2_fun(30);
    }
runTestCase(testcase);
