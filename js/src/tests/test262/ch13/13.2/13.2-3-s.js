











function testcase() {
        "use strict";

        var foo = function () {
            this.arguments = 12;
        } 
        var obj = new foo();
        return obj.arguments === 12;
    }
runTestCase(testcase);
