










function testcase() {
        try {
            throw function () {
                this._12_14_16_foo = "test";
            };
            return false;
        } catch (e) {
            var obj = {};
            obj.test = function () {
                this._12_14_16_foo = "test1";
            };
            e = obj.test;
            e();
            return fnGlobalObject()._12_14_16_foo === "test1";
        }
        finally {
            delete fnGlobalObject()._12_14_16_foo;
        }

    }
runTestCase(testcase);
