











function testcase() {
        "use strict";
        var a = {x:0, get y() { return 0;}};
        delete a.x;
        Object.preventExtensions(a);
        try {
            a.x = 1;
            return false;
        } catch (e) {
            return e instanceof TypeError;
        }
}
runTestCase(testcase);
