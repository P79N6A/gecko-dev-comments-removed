











function testcase() {
        "use strict";
        var objBak = Object;

        try {
            Object = 12;
            return true;
        } finally {
            Object = objBak;
        }
    }
runTestCase(testcase);
