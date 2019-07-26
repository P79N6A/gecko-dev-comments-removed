











function testcase() {
        "use strict";
        var obj = {};
        Object.preventExtensions(obj);

        try {
            obj.len <<= 10;
            return false;
        } catch (e) {
            return e instanceof TypeError;
        }
    }
runTestCase(testcase);
