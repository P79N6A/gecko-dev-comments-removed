











function testcase() {
        "use strict";
        var obj = {};
        Object.defineProperty(obj, "prop", {
            get: function () {
                return 11;
            },
            set: undefined,
            enumerable: true,
            configurable: true
        });

        try {
            obj.prop >>>= 20;
            return false;
        } catch (e) {
            return e instanceof TypeError && obj.prop === 11;
        }
    }
runTestCase(testcase);
