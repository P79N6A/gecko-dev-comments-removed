











function testcase() {
        "use strict";

        var numBak = Number;
        try {
            Number = 12;
            return true;
        } finally {
            Number = numBak;
        }
    }
runTestCase(testcase);
