















function testcase() {
        var accessed = false;
        var strObj = new String("");
        for (var i = 0; strObj;) {
            accessed = true;
            break;
        }
        return accessed;
    }
runTestCase(testcase);
