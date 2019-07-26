












function testcase() {
        var funcExpr = function () { "use strict";};
        return ! Object.getOwnPropertyDescriptor(funcExpr, 
                                                  "caller").configurable;
}
runTestCase(testcase);