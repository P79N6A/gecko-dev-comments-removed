















function testcase() {
        try {
            Object.defineProperty(Object.prototype, "prop", {
                value: 100,
                writable: false,
                configurable: true
            });
            var obj = { prop: 12 };

            return obj.hasOwnProperty("prop") && obj.prop === 12;
        } finally {
            delete Object.prototype.prop;
        }
    }
runTestCase(testcase);
