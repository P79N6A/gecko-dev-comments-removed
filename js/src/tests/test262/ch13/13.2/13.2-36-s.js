












function testcase() {
        var funcExpr = function () { "use strict";};
        return ! Object.getOwnPropertyDescriptor(funcExpr, 
                                                  "arguments").configurable;
}
runTestCase(testcase);