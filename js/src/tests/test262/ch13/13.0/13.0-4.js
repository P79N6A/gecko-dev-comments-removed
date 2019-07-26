










function testcase() {
        var obj = {};
        obj.tt = { len: 10 };
        try {
            eval("function obj.tt.ss() {};");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
