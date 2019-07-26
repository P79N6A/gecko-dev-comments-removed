











function testcase() {
        "use strict";
        arguments[1] = 7;
        ++arguments[1];
        return arguments[1]===8;
    }
runTestCase(testcase);
