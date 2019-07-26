











function testcase() {
        "use strict";

        var foo = function () {
            this.caller = 12;
        }
        var obj = new foo();
        return obj.caller === 12;
    }
runTestCase(testcase);
