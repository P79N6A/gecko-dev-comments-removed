











function testcase() {
        "use strict";
        var _8_7_2_3 = {};
        Object.defineProperty(_8_7_2_3, "b", {
            writable: false
        });

        try {
            _8_7_2_3.b = 11;
            return false;
        } catch (e) {
            return e instanceof TypeError;
        }
    }
runTestCase(testcase);
