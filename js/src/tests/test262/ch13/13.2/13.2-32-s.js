












function testcase() {
        var funcExpr = function () { "use strict";};
        return Object.getOwnPropertyDescriptor(funcExpr, 
                                                  "caller") === undefined;
}
runTestCase(testcase);
